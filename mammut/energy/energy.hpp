/*
 * energy.hpp
 *
 * Created on: 01/12/2014
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

#ifndef ENERGY_HPP_
#define ENERGY_HPP_

#include <mammut/communicator.hpp>
#include <mammut/module.hpp>
#include <mammut/topology/topology.hpp>

namespace mammut{
namespace energy{

typedef double Joules;

class CounterCpu{
private:
    topology::Cpu* _cpu;
    bool _hasJoulesGraphic;
    bool _hasJoulesDram;
protected:
    CounterCpu(topology::Cpu* cpu, bool hasJoulesGraphic, bool hasJoulesDram);
public:
    /**
     * Returns the Cpu associated to this counter.
     * @return The Cpu associated to this counter.
     */
    topology::Cpu* getCpu();

    /**
     * Resets the value of the counter.
     */
    virtual void reset() = 0;

    /**
     * Returns the Joules consumed by the Cpu since the counter creation
     * (or since the last call of reset()).
     * @return Returns the Joules consumed by the Cpu since the counter
     *         creation (or since the last call of reset()).
     */
    virtual Joules getJoules() = 0;

    /**
     * Returns the Joules consumed by the cores of the Cpu since the counter creation
     * (or since the last call of reset()).
     * @return Returns the Joules consumed by the cores of the Cpu since the counter
     *         creation (or since the last call of reset()).
     */
    virtual Joules getJoulesCores() = 0;

    /**
     * Returns true if the counter for integrated graphic card is present, false otherwise.
     * @return True if the counter for integrated graphic card is present, false otherwise.
     */
    virtual bool hasJoulesGraphic();

    /**
     * Returns the Joules consumed by the Cpu integrated graphic card (if present) since
     * the counter creation (or since the last call of reset()).
     * @return Returns the Joules consumed by the Cpu integrated graphic card (if present)
     *         since the counter creation (or since the last call of reset()).
     */
    virtual Joules getJoulesGraphic() = 0;

    /**
     * Returns true if the counter for DRAM is present, false otherwise.
     * @return True if the counter for DRAM is present, false otherwise.
     */
    bool hasJoulesDram();

    /**
     * Returns the Joules consumed by the Cpu Dram since the counter creation
     * (or since the last call of reset()).
     * @return Returns the Joules consumed by the Cpu Dram since the counter
     *         creation (or since the last call of reset()).
     */
    virtual Joules getJoulesDram() = 0;

    virtual ~CounterCpu(){;}
};

class Energy: public Module{
    MAMMUT_MODULE_DECL(Energy)
private:
    std::vector<CounterCpu*> _countersCpu;
    topology::Topology* _topology;
    Energy();
    Energy(Communicator* const communicator);
    ~Energy();
    bool processMessage(const std::string& messageIdIn, const std::string& messageIn,
                                    std::string& messageIdOut, std::string& messageOut);
public:
    /**
     * Returns a vector of counters. Each counter is associated to a Cpu.
     * @return A vector of energy counters of type counterType.
     *         If its size is 0, this type of counters are not
     *         available on this machine.
     */
    std::vector<CounterCpu*> getCountersCpu() const;

    /**
     * Returns the Cpu counter associated to a given cpu identifier.
     * @param cpuId The identifier of the Cpu.
     * @return The Cpu counter associated to cpuId or NULL if it is not present.
     */
    CounterCpu* getCounterCpu(topology::CpuId cpuId) const;
};

}
}

#endif /* ENERGY_HPP_ */
