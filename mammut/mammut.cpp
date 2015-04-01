/*
 * mammut.cpp
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

#include <mammut/mammut.hpp>

namespace mammut{

Mammut::Mammut(Communicator* const communicator):_cpufreq(NULL), _energy(NULL), _topology(NULL),
                                                 _task(NULL), _communicator(communicator){
    ;
}

Mammut::Mammut(const Mammut& other):_cpufreq(NULL), _energy(NULL),
                                    _topology(NULL), _task(NULL){
    this->_communicator = other._communicator;
}

Mammut::~Mammut(){
    releaseModules();
}

void Mammut::releaseModules(){
    if(_cpufreq){
        cpufreq::CpuFreq::release(_cpufreq);
    }

    if(_energy){
        energy::Energy::release(_energy);
    }

    if(_task){
        task::TasksManager::release(_task);
    }

    if(_topology){
        topology::Topology::release(_topology);
    }
}

cpufreq::CpuFreq* Mammut::getInstanceCpuFreq(){
    if(!_cpufreq){
        _cpufreq = cpufreq::CpuFreq::getInstance(_communicator);
    }
    return _cpufreq;
}

energy::Energy* Mammut::getInstanceEnergy(){
    if(!_energy){
        _energy = energy::Energy::getInstance(_communicator);
    }
    return _energy;
}

task::TasksManager* Mammut::getInstanceTask(){
    if(!_task){
        _task = task::TasksManager::getInstance(_communicator);
    }
    return _task;
}

topology::Topology* Mammut::getInstanceTopology(){
    if(!_topology){
        _topology = topology::Topology::getInstance(_communicator);
    }
    return _topology;
}

Mammut& Mammut::operator=(const Mammut& other){
    if(this != &other){
        releaseModules();
        this->_communicator = other._communicator;
    }
    return *this;
}

}
