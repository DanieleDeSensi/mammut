/*
 * energy-linux.hpp
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

#ifndef MAMMUT_ENERGY_LINUX_HPP_
#define MAMMUT_ENERGY_LINUX_HPP_

#include <mammut/energy/energy.hpp>
#include <mammut/topology/topology.hpp>
#include <mammut/external/odroid-smartpower-linux/smartgauge.hpp>

namespace mammut{
namespace energy{

class CounterPlugLinux: public CounterPlug{
private:
    SmartGauge _sg;
    Joules _lastValue;
public:
    CounterPlugLinux();
    bool init();
    Joules getJoules();
    void reset();
};

class CounterCpusLinux;

class CounterCpusLinuxRefresher: public utils::Thread{
private:
    CounterCpusLinux* _counter;
public:
    CounterCpusLinuxRefresher(CounterCpusLinux* counter);
    void run();
};

class CounterCpusLinux: public CounterCpus{
    friend class CounterCpusLinuxRefresher;
private:
    utils::LockPthreadMutex _lock;
    utils::Monitor _stopRefresher;
    CounterCpusLinuxRefresher _refresher;
    utils::Msr** _msrs;
    topology::CpuId _maxId;
    double _powerPerUnit;
    double _energyPerUnit;
    double _timePerUnit;
    double _thermalSpecPower;

    JoulesCpu* _joulesCpus;

    uint32_t* _lastReadCountersCpu;
    uint32_t* _lastReadCountersCores;
    uint32_t* _lastReadCountersGraphic;
    uint32_t* _lastReadCountersDram;

    bool _hasJoulesGraphic;
    bool _hasJoulesDram;

    uint32_t readEnergyCounter(topology::CpuId cpuId, int which);

    /**
     * Returns the minimum number of seconds needed by the counter to wrap.
     * @return The minimum number of seconds needed by the counter to wrap.
     */
    double getWrappingInterval();

    /**
     * Adds to the 'joules' counter the joules consumed from lastReadCounter to
     * the time of this call.
     * @param cpuId The identifier of the CPU.
     * @param joules The joules counter.
     * @param lastReadCounter The last value read for counter counterType.
     * @param counterType The type of the counter.
     */
    void updateCounter(topology::CpuId cpuId, Joules& joules, uint32_t& lastReadCounter, uint32_t counterType);

    static bool isModelSupported(std::string model);
    static bool hasGraphicCounter(topology::Cpu* cpu);
    static bool hasDramCounter(topology::Cpu* cpu);
    static bool isCpuSupported(topology::Cpu* cpu);
public:
    CounterCpusLinux();
    ~CounterCpusLinux();

    JoulesCpu getJoulesComponents(topology::CpuId cpuId);
    Joules getJoulesCpu(topology::CpuId cpuId);
    Joules getJoulesCores(topology::CpuId cpuId);
    Joules getJoulesGraphic(topology::CpuId cpuId);
    Joules getJoulesDram(topology::CpuId cpuId);
    bool hasJoulesDram();
    bool hasJoulesGraphic();
    bool init();
    void reset();
};

}
}

#endif /* MAMMUT_ENERGY_LINUX_HPP_ */
