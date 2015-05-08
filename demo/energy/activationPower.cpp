/*
 * activationPower.cpp
 * Computes the power needed to activate one voltage domain.
 *
 * Created on: 07/05/2015
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

#include <iostream>
#include <unistd.h>

double seconds = 20;

int main(int argc, char** argv){
    mammut::Mammut mammut;
    mammut::energy::Energy* energy = mammut.getInstanceEnergy();
    mammut::cpufreq::CpuFreq* frequency = mammut.getInstanceCpuFreq();
    std::vector<mammut::cpufreq::Domain*> domains = frequency->getDomains();
    assert(domains.size());
    mammut::cpufreq::Domain* domain = domains.at(0);
    mammut::topology::VirtualCore* vc = domain->getVirtualCores().at(0);
    std::vector<mammut::cpufreq::Frequency> availableFrequencies = domain->getAvailableFrequencies();
    mammut::energy::CounterCpu* counter = energy->getCounterCpu(vc->getCpuId());
    std::vector<mammut::topology::PhysicalCore*> physicalCores = mammut.getInstanceTopology()->virtualToPhysical(domain->getVirtualCores());

    mammut::cpufreq::RollbackPoint rp = domain->getRollbackPoint();
    domain->setGovernor(mammut::cpufreq::GOVERNOR_USERSPACE);

    for(size_t i = 0; i < availableFrequencies.size(); i++){
        domain->setFrequencyUserspace(availableFrequencies.at(i));
        std::cout << "data = {";
        for(size_t j = 0; j < physicalCores.size(); j++){
            std::cout << "{" << j+1 << ",";
            for(size_t k = 0; k <= j; k++){
                physicalCores.at(k)->getVirtualCore()->maximizeUtilization();
            }
            counter->reset();
            sleep(seconds);
            std::cout << counter->getJoulesCores()/seconds << "}";
            if(j < physicalCores.size() - 1){
                std::cout << ",";
            }
            for(size_t k = 0; k <= j; k++){
                physicalCores.at(k)->getVirtualCore()->resetUtilization();
            }
        }
        std::cout << "}" << std::endl;
        std::cout << "lm = LinearModelFit[data, x, x]" << std::endl;
    }

    domain->rollback(rp);
}
