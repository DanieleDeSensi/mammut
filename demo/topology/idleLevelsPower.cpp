/*
 * idleLevelsPower.cpp
 * This is a demo on idle levels power consumption.
 *
 * Created on: 08/04/2015
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
#include <mammut/communicator-tcp.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

const double levelTime = 10;

int main(int argc, char** argv){

    if(argc < 2){
        std::cerr << "Usage: " << argv[0] << " CpuId" << std::endl;
    }

    unsigned int cpuId = atoi(argv[1]);

    mammut::Mammut mammut;

    /*******************************************/
    /*       Idle states power consumption     */
    /*******************************************/
    mammut::topology::VirtualCore* vc = mammut.getInstanceTopology()->getCpus().at(cpuId)->getVirtualCore();
    mammut::topology::Cpu* cpu = mammut.getInstanceTopology()->getCpu(vc->getCpuId());
    std::vector<mammut::topology::VirtualCore*> virtualCores = cpu->getVirtualCores();
    std::vector<mammut::topology::VirtualCoreIdleLevel*> idleLevels = vc->getIdleLevels();
    mammut::cpufreq::Domain* fDomain = mammut.getInstanceCpuFreq()->getDomain(vc);
    if(idleLevels.size() == 0){
        std::cout << "No idle levels supported by CPU " << cpu->getCpuId() << "." << std::endl;
    }else{
        for(int32_t i = idleLevels.size() - 1; i >= 0 ; i--){
            int fd = open("/dev/cpu_dma_latency", O_RDWR);
            int32_t lat = idleLevels.at(i)->getExitLatency();
            write(fd, &lat, sizeof(lat));

            /** We compute the base power consumption at each frequency step. **/
            std::vector<mammut::cpufreq::Frequency> frequencies;
            frequencies = fDomain->getAvailableFrequencies();

            for(size_t j = 0; j < frequencies.size(); j++){
                fDomain->setGovernor(mammut::cpufreq::GOVERNOR_USERSPACE);
                fDomain->setFrequencyUserspace(frequencies.at(j));
                mammut::energy::CounterCpu* counter = mammut.getInstanceEnergy()->getCounterCpu(cpu->getCpuId());
                counter->reset();
                sleep(levelTime);

                std::cout << idleLevels.at(i)->getName() << " ";
                std::cout << frequencies.at(j) << " ";
                std::cout << counter->getJoulesCpu()/levelTime << " ";
                std::cout << counter->getJoulesCores()/levelTime << " ";
                std::cout << counter->getJoulesDram()/levelTime << " ";
                std::cout << counter->getJoulesGraphic()/levelTime << " ";
                std::cout << std::endl;
            }

            close(fd);
        }
    }
}
