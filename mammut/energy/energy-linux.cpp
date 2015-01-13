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

#define MSR_RAPL_POWER_UNIT 0x606

/*
 * Platform specific RAPL Domains.
 * Note that PP1 RAPL Domain is supported on 062A only
 * And DRAM RAPL Domain is supported on 062D only
 */
/* Package RAPL Domain */
#define MSR_PKG_RAPL_POWER_LIMIT 0x610
#define MSR_PKG_ENERGY_STATUS 0x611
#define MSR_PKG_PERF_STATUS 0x613
#define MSR_PKG_POWER_INFO 0x614

/* PP0 RAPL Domain */
#define MSR_PP0_POWER_LIMIT 0x638
#define MSR_PP0_ENERGY_STATUS 0x639
#define MSR_PP0_POLICY 0x63A
#define MSR_PP0_PERF_STATUS 0x63B

/* PP1 RAPL Domain, may reflect to uncore devices */
#define MSR_PP1_POWER_LIMIT 0x640
#define MSR_PP1_ENERGY_STATUS 0x641
#define MSR_PP1_POLICY 0x642

/* DRAM RAPL Domain */
#define MSR_DRAM_POWER_LIMIT 0x618
#define MSR_DRAM_ENERGY_STATUS 0x619
#define MSR_DRAM_PERF_STATUS 0x61B
#define MSR_DRAM_POWER_INFO 0x61C

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
        _counter->getJoules();
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
    return !cpu->getFamily().compare("6") &&
           !cpu->getVendorId().compare(0,12,"GenuineIntel") &&
           isModelSupported(cpu->getModel());
}

CounterCpuLinux::CounterCpuLinux(topology::Cpu* cpu):
        CounterCpu(cpu, hasGraphicCounter(cpu), hasDramCounter(cpu)),
        _lock(),
        _stopRefresher(),
        _refresher(this){
    std::string msrFileName = "/dev/cpu/" + utils::intToString(cpu->getVirtualCore()->getVirtualCoreId()) + "/msr";
    _fd = open(msrFileName.c_str(), O_RDONLY);
    if(_fd == -1){
        throw std::runtime_error("Impossible to open /dev/cpu/*/msr: " + utils::errnoToStr());
    }
    /* Calculate the units used */
    uint64_t result = readMsr(MSR_RAPL_POWER_UNIT);
    _powerPerUnit = pow(0.5,(double)(result&0xF));
    _energyPerUnit = pow(0.5,(double)((result>>8)&0x1F));
    _timePerUnit = pow(0.5,(double)((result>>16)&0xF));
    result = readMsr(MSR_PKG_POWER_INFO);
    _thermalSpecPower = _powerPerUnit*(double)(result&0x7FFF);
    reset();
    _refresher.start();
}

CounterCpuLinux::~CounterCpuLinux(){
    _stopRefresher.notifyAll();
    _refresher.join();
    close(_fd);
}

uint64_t CounterCpuLinux::readMsr(int which) {
    uint64_t data;
    if(pread(_fd, (void*) &data, sizeof(data), (off_t) which) != sizeof(data)){
        throw std::runtime_error("Error while reading msr register: " + utils::errnoToStr());
    }
    return data;
}

uint32_t CounterCpuLinux::readEnergyCounter(int which){
    switch(which){
        case MSR_PKG_ENERGY_STATUS:
        case MSR_PP0_ENERGY_STATUS:
        case MSR_PP1_ENERGY_STATUS:
        case MSR_DRAM_ENERGY_STATUS:{
            return readMsr(which) & 0xFFFFFFFF;
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

double CounterCpuLinux::updateCounter(double& joules, uint32_t& lastReadCounter, uint32_t counterType){
    uint32_t currentCounter = readEnergyCounter(counterType);
    joules += ((double) deltaDiff(lastReadCounter, currentCounter)) * _energyPerUnit;
    lastReadCounter = currentCounter;
    return joules;
}


Joules CounterCpuLinux::getJoules(){
    utils::ScopedLock sLock(_lock);
    return updateCounter(_joules, _lastReadCounter, MSR_PKG_ENERGY_STATUS);
}

Joules CounterCpuLinux::getJoulesCores(){
    utils::ScopedLock sLock(_lock);
    return updateCounter(_joulesCores, _lastReadCounterCores, MSR_PP0_ENERGY_STATUS);
}

Joules CounterCpuLinux::getJoulesGraphic(){
    if(hasJoulesGraphic()){
        utils::ScopedLock sLock(_lock);
        return updateCounter(_joulesGraphic, _lastReadCounterGraphic, MSR_PP1_ENERGY_STATUS);
    }else{
        return 0;
    }
}

Joules CounterCpuLinux::getJoulesDram(){
    if(hasJoulesDram()){
        utils::ScopedLock sLock(_lock);
        return updateCounter(_joulesDram, _lastReadCounterDram, MSR_DRAM_ENERGY_STATUS);
    }else{
        return 0;
    }
}

void CounterCpuLinux::reset(){
    _lastReadCounter = readEnergyCounter(MSR_PKG_ENERGY_STATUS);
    _lastReadCounterCores = readEnergyCounter(MSR_PP0_ENERGY_STATUS);
    if(hasJoulesGraphic()){
        _lastReadCounterGraphic = readEnergyCounter(MSR_PP1_ENERGY_STATUS);
    }
    if(hasJoulesDram()){
        _lastReadCounterDram = readEnergyCounter(MSR_DRAM_ENERGY_STATUS);
    }
    _joules = 0;
    _joulesCores = 0;
    _joulesGraphic = 0;
    _joulesDram = 0;
}

}
}
