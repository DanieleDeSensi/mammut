/*
 * mammut.hpp
 *
 * Created on: 27/03/2015
 *
 * =========================================================================
 *  Copyright (C) 2015-, Daniele De Sensi (d.desensi.software@gmail.com)
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

#ifndef MAMMUT_MAMMUT_HPP_
#define MAMMUT_MAMMUT_HPP_

#include "./cpufreq/cpufreq.hpp"
#include "./energy/energy.hpp"
#include "./task/task.hpp"
#include "./topology/topology.hpp"

namespace mammut{

class Mammut{
private:
    mutable cpufreq::CpuFreq* _cpufreq;
    mutable energy::Energy* _energy;
    mutable topology::Topology* _topology;
    mutable task::TasksManager* _task;

    Communicator* _communicator;

    /**
     * Release all the modules instances.
     */
    void releaseModules();
public:
    //TODO: Rule of the three

    /**
     * Creates a mammut handler.
     * @param communicator If different from NULL, all the modules will interact with
     *        a remote machine specified in the communicator.
     */
    Mammut(Communicator* const communicator = NULL);

    /**
     * Copy constructor.
     * @param other The object to copy from.
     */
    Mammut(const Mammut& other);

    /**
     * Destroyes this mammut instance.
     */
    ~Mammut();

    /**
     * Assigns an instance of mammut to another one.
     * @param other The other instance.
     */
    Mammut& operator=(const Mammut& other);

    /**
     * Returns an instance of the CpuFreq module.
     * @return An instance of the CpuFreq module.
     */
    cpufreq::CpuFreq* getInstanceCpuFreq() const;

    /**
     * Returns an instance of the Energy module.
     * @return An instance of the Energy module.
     */
    energy::Energy* getInstanceEnergy() const;

    /**
     * Returns an instance of the Task module.
     * @return An instance of the Task module.
     */
    task::TasksManager* getInstanceTask() const;

    /**
     * Returns an instance of the Topology module.
     * @return An instance of the Topology module.
     */
    topology::Topology* getInstanceTopology() const;


};

}



#endif /* MAMMUT_MAMMUT_HPP_ */
