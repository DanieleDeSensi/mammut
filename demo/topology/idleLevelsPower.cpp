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
    mammut::CommunicatorTcp* communicator = NULL;
    std::cout << "Usage: " << argv[0] << " [TcpAddress:TcpPort]" << std::endl;

    /** Gets the address and the port of the server and builds the communicator. **/
    if(argc > 1){
        std::string addressPort = argv[1];
        size_t pos = addressPort.find_first_of(":");
        std::string address = addressPort.substr(0, pos);
        uint16_t port = atoi(addressPort.substr(pos + 1).c_str());
        communicator = new mammut::CommunicatorTcp(address, port);
    }

    mammut::Mammut mammut(communicator);

    /*******************************************/
    /*       Idle states power consumption     */
    /*******************************************/
    mammut::topology::VirtualCore* vc = mammut.getInstanceTopology()->getCpus().back()->getVirtualCore();
    mammut::topology::Cpu* cpu = mammut.getInstanceTopology()->getCpu(vc->getCpuId());
    std::vector<mammut::topology::VirtualCore*> virtualCores = cpu->getVirtualCores();
    std::vector<mammut::topology::VirtualCoreIdleLevel*> idleLevels = vc->getIdleLevels();
    mammut::cpufreq::Domain* fDomain = mammut.getInstanceCpuFreq()->getDomain(vc);
    if(idleLevels.size() == 0){
        std::cout << "No idle levels supported by CPU " << cpu->getCpuId() << "." << std::endl;
    }else{
        int fd = open("/dev/cpu_dma_latency", O_RDWR);
        for(size_t i = 0; i < idleLevels.size(); i++){
            uint32_t lat = idleLevels.at(i)->getExitLatency();
            write(fd, &lat, sizeof(lat));

#if 0
            for(size_t j = 0; j < virtualCores.size(); j++){
                std::vector<mammut::topology::VirtualCoreIdleLevel*> tmpLevels = virtualCores.at(j)->getIdleLevels();
                for(size_t k = 0; k < tmpLevels.size(); k++){
                    if(k != i){
                        tmpLevels.at(k)->disable();
                    }
                }
            }
#endif

            /** For C0, we compute the base power consumption at each frequency step. **/
            std::vector<mammut::cpufreq::Frequency> frequencies;
            if(i == 0){
                frequencies = fDomain->getAvailableFrequencies();
            }else{
                frequencies.push_back(0);
            }

            for(size_t j = 0; j < frequencies.size(); j++){
                if(frequencies.at(j)){
                    fDomain->setGovernor(mammut::cpufreq::GOVERNOR_USERSPACE);
                    fDomain->setFrequencyUserspace(frequencies.at(j));
                }
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

#if 0
            for(size_t j = 0; j < virtualCores.size(); j++){
                std::vector<mammut::topology::VirtualCoreIdleLevel*> tmpLevels = virtualCores.at(j)->getIdleLevels();
                for(size_t k = 0; k < tmpLevels.size(); k++){
                    if(k != i){
                        tmpLevels.at(k)->enable();
                    }
                }
            }
#endif
        }
        close(fd);
    }
}
