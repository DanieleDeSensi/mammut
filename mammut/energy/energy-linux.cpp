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

#define JOULES_IN_WATTHOUR 3600.0

namespace mammut{
namespace energy{

CounterPlugLinux::CounterPlugLinux():_lastValue(0){
    ;
}

bool CounterPlugLinux::init(){
    bool available = _sg.initDevice();
    if(available){
        _lastValue = getJoulesAbs();
    }
    return available;
}

Joules CounterPlugLinux::getJoulesAbs(){
    return (_sg.getWattHour() * JOULES_IN_WATTHOUR);
}
  
Joules CounterPlugLinux::getJoules(){
    return getJoulesAbs() - _lastValue;
}

void CounterPlugLinux::reset(){
    _lastValue = getJoulesAbs();
}

CounterCpusLinuxRefresher::CounterCpusLinuxRefresher(CounterCpusLinux* counter):_counter(counter){
    ;
}

void CounterCpusLinuxRefresher::run(){
    double sleepingIntervalMs = (_counter->getWrappingInterval() / 2) * 1000;
    while(!_counter->_stopRefresher.timedWait(sleepingIntervalMs)){
        _counter->getJoulesComponentsAll();
    }
}

bool CounterCpusLinux::isModelSupported(std::string model){
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

bool CounterCpusLinux::hasGraphicCounter(topology::Cpu* cpu){
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

bool CounterCpusLinux::hasDramCounter(topology::Cpu* cpu){
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

bool CounterCpusLinux::isCpuSupported(topology::Cpu* cpu){
    utils::Msr msr(cpu->getVirtualCore()->getVirtualCoreId());
    uint64_t dummy;
    return !cpu->getFamily().compare("6") &&
           !cpu->getVendorId().compare(0,12,"GenuineIntel") &&
           isModelSupported(cpu->getModel()) &&
           msr.available() &&
           msr.read(MSR_RAPL_POWER_UNIT, dummy) && dummy &&
           msr.read(MSR_PKG_POWER_INFO, dummy) && dummy;
}

CounterCpusLinux::CounterCpusLinux():
        CounterCpus(topology::Topology::getInstance()),
        _initialized(false),
        _lock(),
        _stopRefresher(),
        _refresher(this),
        _msrs(NULL),
        _maxId(0),
        _powerPerUnit(0),
        _energyPerUnit(0),
        _timePerUnit(0),
        _thermalSpecPower(0),
        _joulesCpus(NULL),
        _lastReadCountersCpu(NULL),
        _lastReadCountersCores(NULL),
        _lastReadCountersGraphic(NULL),
        _lastReadCountersDram(NULL),
        _hasJoulesGraphic(false),
        _hasJoulesDram(false){
    ;
}

bool CounterCpusLinux::init(){
    for(size_t i = 0; i < _cpus.size(); i++){
        if(!isCpuSupported(_cpus.at(i))){
            return false;
        }	
    }

    _initialized = true;
    /**
     * I have one msr for each CPU. Since I have no guarantee that the CPU
     * identifiers are contiguous, I find their maximum id to decide
     * how long the array of msrs should be.
     * For example, if a have a CPU with id 1 and a CPU with id 3, then
     * I will allocate an array of 4 positions.
     * In msrs[1] I will have the msr associated to CPU 1 and in msrs[3]
     * I will have the msr associated to CPU 3.
     */
    _maxId = 0;
    topology::Cpu* cpu;
    for(size_t i = 0; i < _cpus.size(); i++){
        cpu = _cpus.at(i);
        if(cpu->getCpuId() > _maxId){
            _maxId = cpu->getCpuId();
        }
    }

    _msrs = new utils::Msr*[_maxId + 1];
    for(size_t i = 0; i < _cpus.size(); i++){
        cpu = _cpus.at(i);
        _msrs[cpu->getCpuId()] = new utils::Msr(cpu->getVirtualCore()->getVirtualCoreId());
        if(!_msrs[cpu->getCpuId()]->available()){
            throw std::runtime_error("Impossible to open msr for CPU " +
                                     utils::intToString(cpu->getCpuId()));
        }
    }

    _joulesCpus = new JoulesCpu[_maxId + 1];

    _lastReadCountersCpu = new uint32_t[_maxId + 1];
    _lastReadCountersCores = new uint32_t[_maxId + 1];
    _lastReadCountersGraphic = new uint32_t[_maxId + 1];
    _lastReadCountersDram = new uint32_t[_maxId + 1];

    /*
     * Calculate the units used. We suppose to have the same units
     * for all the CPUs. So we can read the units of any of the CPUs.
     */
    uint64_t result;
    _msrs[_maxId]->read(MSR_RAPL_POWER_UNIT, result);
    _powerPerUnit = pow(0.5,(double)(result&0xF));
    _energyPerUnit = pow(0.5,(double)((result>>8)&0x1F));
    _timePerUnit = pow(0.5,(double)((result>>16)&0xF));
    _msrs[_maxId]->read(MSR_PKG_POWER_INFO, result);
    _thermalSpecPower = _powerPerUnit*(double)(result&0x7FFF);

    _hasJoulesDram = true;
    _hasJoulesGraphic = true;
    for(size_t i = 0; i < _cpus.size(); i++){
        if(!hasDramCounter(_cpus.at(i))){
            _hasJoulesDram = false;
        }
        if(!hasGraphicCounter(_cpus.at(i))){
            _hasJoulesGraphic = false;
        }
    }

    reset();
    _refresher.start();
    return true;
}

CounterCpusLinux::~CounterCpusLinux(){
    if(_initialized){
        _stopRefresher.notifyAll();
        _refresher.join();

        for(size_t i = 0; i < _maxId; i++){
            delete _msrs[i];
        }
        delete[] _msrs;

        delete[] _joulesCpus;
        delete[] _lastReadCountersCpu;
        delete[] _lastReadCountersCores;
        delete[] _lastReadCountersGraphic;
        delete[] _lastReadCountersDram;
    }
}

uint32_t CounterCpusLinux::readEnergyCounter(topology::CpuId cpuId, int which){
    switch(which){
        case MSR_PKG_ENERGY_STATUS:
        case MSR_PP0_ENERGY_STATUS:
        case MSR_PP1_ENERGY_STATUS:
        case MSR_DRAM_ENERGY_STATUS:{
            uint64_t result;
            if(!_msrs[cpuId]->read(which, result) || !result){
                throw std::runtime_error("Fatal error. Counter has been created but registers are not present.");
            }
            return result & 0xFFFFFFFF;
        }break;
        default:{
            throw std::runtime_error("Invalid energy counter specification.");
        }
    }
}

double CounterCpusLinux::getWrappingInterval(){
    return ((double) 0xFFFFFFFF) * _energyPerUnit / _thermalSpecPower;
}

static uint32_t deltaDiff(uint32_t c1, uint32_t c2){
    if(c2 > c1){
        return c2 - c1;
    }else{
        return (((uint32_t)0xFFFFFFFF) - c1) + (uint32_t)1 + c2;
    }
}

void CounterCpusLinux::updateCounter(topology::CpuId cpuId, double& joules, uint32_t& lastReadCounter, uint32_t counterType){
    uint32_t currentCounter = readEnergyCounter(cpuId, counterType);
    joules += ((double) deltaDiff(lastReadCounter, currentCounter)) * _energyPerUnit;
    lastReadCounter = currentCounter;
}

JoulesCpu CounterCpusLinux::getJoulesComponents(topology::CpuId cpuId){
	return JoulesCpu(getJoulesCpu(cpuId), getJoulesCores(cpuId), getJoulesGraphic(cpuId), getJoulesDram(cpuId));
}

Joules CounterCpusLinux::getJoulesCpu(topology::CpuId cpuId){
    utils::ScopedLock sLock(_lock);
    updateCounter(cpuId, _joulesCpus[cpuId].cpu, _lastReadCountersCpu[cpuId], MSR_PKG_ENERGY_STATUS);
    return _joulesCpus[cpuId].cpu;
}

Joules CounterCpusLinux::getJoulesCores(topology::CpuId cpuId){
    utils::ScopedLock sLock(_lock);
    updateCounter(cpuId, _joulesCpus[cpuId].cores, _lastReadCountersCores[cpuId], MSR_PP0_ENERGY_STATUS);
    return _joulesCpus[cpuId].cores;
}

Joules CounterCpusLinux::getJoulesGraphic(topology::CpuId cpuId){
    if(hasJoulesGraphic()){
        utils::ScopedLock sLock(_lock);
        updateCounter(cpuId, _joulesCpus[cpuId].graphic, _lastReadCountersGraphic[cpuId], MSR_PP1_ENERGY_STATUS);
        return _joulesCpus[cpuId].graphic;
    }else{
        return 0;
    }
}

Joules CounterCpusLinux::getJoulesDram(topology::CpuId cpuId){
    if(hasJoulesDram()){
        utils::ScopedLock sLock(_lock);
        updateCounter(cpuId, _joulesCpus[cpuId].dram, _lastReadCountersDram[cpuId], MSR_DRAM_ENERGY_STATUS);
        return _joulesCpus[cpuId].dram;
    }else{
        return 0;
    }
}

void CounterCpusLinux::reset(){
    for(size_t i = 0; i < _cpus.size(); i++){
        _lastReadCountersCpu[i] = readEnergyCounter(i, MSR_PKG_ENERGY_STATUS);
        _lastReadCountersCores[i] = readEnergyCounter(i, MSR_PP0_ENERGY_STATUS);
        if(hasJoulesGraphic()){
            _lastReadCountersGraphic[i] = readEnergyCounter(i, MSR_PP1_ENERGY_STATUS);
        }
        if(hasJoulesDram()){
            _lastReadCountersDram[i] = readEnergyCounter(i, MSR_DRAM_ENERGY_STATUS);
        }
        _joulesCpus[i].cpu = 0;
        _joulesCpus[i].cores = 0;
        _joulesCpus[i].graphic = 0;
        _joulesCpus[i].dram = 0;
    }
}

bool CounterCpusLinux::hasJoulesDram(){
    return _hasJoulesDram;
}

bool CounterCpusLinux::hasJoulesGraphic(){
    return _hasJoulesGraphic;
}

}
}
