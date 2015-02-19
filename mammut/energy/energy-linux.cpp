/*
 * energy-linux.cpp
 *
 * Created on: 02/12/2014
 *
 * =========================================================================
 *  Copyright (C) 2014-, Daniele De Sensi (d.desensi.software@gmail.com)
 *
 *  This file is part of mammut.
 *
 *  mammut is free software: you can redistribute it and/or
 *  modify it under the terms of the Lesser GNU General Public
 *  License as published by the Free Software Foundation, either
 *  version 3 of the License, or (at your option) any later version.

 *  mammut is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  Lesser GNU General Public License for more details.
 *
 *  You should have received a copy of the Lesser GNU General Public
 *  License along with mammut.
 *  If not, see <http://www.gnu.org/licenses/>.
 *
 * =========================================================================
 */

#include <mammut/energy/energy-linux.hpp>

#include <cmath>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>

/* RAPL UNIT BITMASK */
#define POWER_UNIT_OFFSET 0
#define POWER_UNIT_MASK 0x0F

#define ENERGY_UNIT_OFFSET 0x08
#define ENERGY_UNIT_MASK 0x1F00

#define TIME_UNIT_OFFSET 0x10
#define TIME_UNIT_MASK 0xF000

#define CPU_SANDYBRIDGE 42
#define CPU_SANDYBRIDGE_EP 45
#define CPU_IVYBRIDGE 58
#define CPU_IVYBRIDGE_EP 62
#define CPU_HASWELL 60
#define CPU_HASWELL_EP 63

namespace mammut{
namespace energy{

CounterCpuLinuxRefresher::CounterCpuLinuxRefresher(CounterCpuLinux* counter):_counter(counter){
    ;
}

void CounterCpuLinuxRefresher::run(){
    double sleepingIntervalMs = (_counter->getWrappingInterval() / 2) * 1000;
    while(!_counter->_stopRefresher.timedWait(sleepingIntervalMs)){
        _counter->getJoulesCpu();
        _counter->getJoulesCores();
        _counter->getJoulesGraphic();
        _counter->getJoulesDram();
    }
}

bool CounterCpuLinux::isModelSupported(std::string model){
    uint32_t modelInt = utils::stringToInt(model);
    switch (modelInt){
        case CPU_SANDYBRIDGE:
        case CPU_IVYBRIDGE:
        case CPU_SANDYBRIDGE_EP:
        case CPU_IVYBRIDGE_EP:
        case CPU_HASWELL_EP:
        case CPU_HASWELL:
        case 69:
        case 79:{
            return true;
        }break;
    }
    return false;
}

bool CounterCpuLinux::hasGraphicCounter(topology::Cpu* cpu){
    uint32_t modelInt = utils::stringToInt(cpu->getModel());
    switch (modelInt){
        case CPU_SANDYBRIDGE:
        case CPU_IVYBRIDGE:
        case CPU_HASWELL:
        case 69:
        case 79:{
            return true;
        }break;
    }
    return false;
}

bool CounterCpuLinux::hasDramCounter(topology::Cpu* cpu){
    uint32_t modelInt = utils::stringToInt(cpu->getModel());
    switch (modelInt){
        case CPU_SANDYBRIDGE_EP:
        case CPU_IVYBRIDGE_EP:
        case CPU_HASWELL_EP:{
            return true;
        }break;
        case CPU_HASWELL:
        case 69:
        case 79:{
            /* Not documented by Intel but seems to work */
            return true;
        }
    }
    return false;

}

bool CounterCpuLinux::isCpuSupported(topology::Cpu* cpu){
    utils::Msr msr(cpu->getVirtualCore()->getVirtualCoreId());
    uint64_t dummy;
    return !cpu->getFamily().compare("6") &&
           !cpu->getVendorId().compare(0,12,"GenuineIntel") &&
           isModelSupported(cpu->getModel()) &&
           msr.available() &&
           msr.read(MSR_RAPL_POWER_UNIT, dummy) && dummy &&
           msr.read(MSR_PKG_POWER_INFO, dummy) && dummy;
}

CounterCpuLinux::CounterCpuLinux(topology::Cpu* cpu):
        CounterCpu(cpu, hasGraphicCounter(cpu), hasDramCounter(cpu)),
        _lock(),
        _stopRefresher(),
        _refresher(this),
        _msr(cpu->getVirtualCore()->getVirtualCoreId()){
    if(!_msr.available()){
        throw std::runtime_error("Impossible to open msr for virtual core " + utils::intToString(cpu->getVirtualCore()->getVirtualCoreId()));
    }

    /* Calculate the units used */
    uint64_t result;
    _msr.read(MSR_RAPL_POWER_UNIT, result);
    _powerPerUnit = pow(0.5,(double)(result&0xF));
    _energyPerUnit = pow(0.5,(double)((result>>8)&0x1F));
    _timePerUnit = pow(0.5,(double)((result>>16)&0xF));
    _msr.read(MSR_PKG_POWER_INFO, result);
    _thermalSpecPower = _powerPerUnit*(double)(result&0x7FFF);
    reset();
    _refresher.start();
}

CounterCpuLinux::~CounterCpuLinux(){
    _stopRefresher.notifyAll();
    _refresher.join();
}

uint32_t CounterCpuLinux::readEnergyCounter(int which){
    switch(which){
        case MSR_PKG_ENERGY_STATUS:
        case MSR_PP0_ENERGY_STATUS:
        case MSR_PP1_ENERGY_STATUS:
        case MSR_DRAM_ENERGY_STATUS:{
            uint64_t result;
            if(!_msr.read(which, result) || !result){
                throw std::runtime_error("Fatal error. Counter has been created but registers are not present.");
            }
            return result & 0xFFFFFFFF;
        }break;
        default:{
            throw std::runtime_error("Invalid energy counter specification.");
        }
    }
}

double CounterCpuLinux::getWrappingInterval(){
    return ((double) 0xFFFFFFFF) * _energyPerUnit / _thermalSpecPower;
}

static uint32_t deltaDiff(uint32_t c1, uint32_t c2){
    if(c2 > c1){
        return c2 - c1;
    }else{
        return (((uint32_t)0xFFFFFFFF) - c1) + (uint32_t)1 + c2;
    }
}

void CounterCpuLinux::updateCounter(double& joules, uint32_t& lastReadCounter, uint32_t counterType){
    uint32_t currentCounter = readEnergyCounter(counterType);
    joules += ((double) deltaDiff(lastReadCounter, currentCounter)) * _energyPerUnit;
    lastReadCounter = currentCounter;
}

JoulesCpu CounterCpuLinux::getJoules(){
	return JoulesCpu(getJoulesCpu(), getJoulesCores(), getJoulesGraphic(), getJoulesDram());
}

Joules CounterCpuLinux::getJoulesCpu(){
    utils::ScopedLock sLock(_lock);
    updateCounter(_joulesCpu.cpu, _lastReadCounterCpu, MSR_PKG_ENERGY_STATUS);
    return _joulesCpu.cpu;
}

Joules CounterCpuLinux::getJoulesCores(){
    utils::ScopedLock sLock(_lock);
    updateCounter(_joulesCpu.cores, _lastReadCounterCores, MSR_PP0_ENERGY_STATUS);
    return _joulesCpu.cores;
}

Joules CounterCpuLinux::getJoulesGraphic(){
    if(hasJoulesGraphic()){
        utils::ScopedLock sLock(_lock);
        updateCounter(_joulesCpu.graphic, _lastReadCounterGraphic, MSR_PP1_ENERGY_STATUS);
        return _joulesCpu.graphic;
    }else{
        return 0;
    }
}

Joules CounterCpuLinux::getJoulesDram(){
    if(hasJoulesDram()){
        utils::ScopedLock sLock(_lock);
        updateCounter(_joulesCpu.dram, _lastReadCounterDram, MSR_DRAM_ENERGY_STATUS);
        return _joulesCpu.dram;
    }else{
        return 0;
    }
}

void CounterCpuLinux::reset(){
    _lastReadCounterCpu = readEnergyCounter(MSR_PKG_ENERGY_STATUS);
    _lastReadCounterCores = readEnergyCounter(MSR_PP0_ENERGY_STATUS);
    if(hasJoulesGraphic()){
        _lastReadCounterGraphic = readEnergyCounter(MSR_PP1_ENERGY_STATUS);
    }
    if(hasJoulesDram()){
        _lastReadCounterDram = readEnergyCounter(MSR_DRAM_ENERGY_STATUS);
    }
    _joulesCpu.cpu = 0;
    _joulesCpu.cores = 0;
    _joulesCpu.graphic = 0;
    _joulesCpu.dram = 0;
}

}
}
