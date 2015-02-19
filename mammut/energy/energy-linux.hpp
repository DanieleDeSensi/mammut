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

namespace mammut{
namespace energy{

class CounterCpuLinux;

class CounterCpuLinuxRefresher: public utils::Thread{
private:
    CounterCpuLinux* _counter;
public:
    CounterCpuLinuxRefresher(CounterCpuLinux* counter);
    void run();
};

class CounterCpuLinux: public CounterCpu{
    friend class CounterCpuLinuxRefresher;
private:
    utils::LockPthreadMutex _lock;
    utils::Monitor _stopRefresher;
    CounterCpuLinuxRefresher _refresher;
    utils::Msr _msr;
    double _powerPerUnit;
    double _energyPerUnit;
    double _timePerUnit;
    double _thermalSpecPower;

    JoulesCpu _joulesCpu;

    uint32_t _lastReadCounterCpu;
    uint32_t _lastReadCounterCores;
    uint32_t _lastReadCounterGraphic;
    uint32_t _lastReadCounterDram;

    uint32_t readEnergyCounter(int which);

    /**
     * Returns the minimum number of seconds needed by the counter to wrap.
     * @return The minimum number of seconds needed by the counter to wrap.
     */
    double getWrappingInterval();

    /**
     * Adds to the 'joules' counter the joules consumed from lastReadCounter to
     * the time of this call.
     * @param joules The joules counter.
     * @param lastReadCounter The last value read for counter counterType.
     * @param counterType The type of the counter.
     */
    void updateCounter(Joules& joules, uint32_t& lastReadCounter, uint32_t counterType);
    static bool isModelSupported(std::string model);
    static bool hasGraphicCounter(topology::Cpu* cpu);
    static bool hasDramCounter(topology::Cpu* cpu);
public:
    static bool isCpuSupported(topology::Cpu* cpu);
    CounterCpuLinux(topology::Cpu* cpu);
    ~CounterCpuLinux();
    void reset();
    JoulesCpu getJoules();
    Joules getJoulesCpu();
    Joules getJoulesCores();
    Joules getJoulesGraphic();
    Joules getJoulesDram();
};

}
}

#endif /* MAMMUT_ENERGY_LINUX_HPP_ */
