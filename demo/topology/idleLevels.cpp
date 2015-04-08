/*
 * idleLevels.cpp
 * This is a demo on idle levels structure and power consumption.
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

#include <iostream>
#include <unistd.h>

const unsigned int levelTime = 10;

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
    /*              Idle states test           */
    /*******************************************/
    mammut::topology::VirtualCore* vc = mammut.getInstanceTopology()->getVirtualCore(0);
    mammut::topology::Cpu* cpu = mammut.getInstanceTopology()->getCpu(vc->getCpuId());
    std::vector<mammut::topology::VirtualCoreIdleLevel*> idleLevels = vc->getIdleLevels();
    if(idleLevels.size() == 0){
        std::cout << "No idle levels supported by CPU " << cpu->getCpuId() << "." << std::endl;
    }else{
        std::cout << "The following idle levels are supported by CPU " << cpu->getCpuId() << ":" << std::endl;
        for(size_t i = 0; i < idleLevels.size(); i++){
            mammut::topology::VirtualCoreIdleLevel* level = idleLevels.at(i);
            std::cout << "[Idle Level: " << level->getLevelId() << "]";
            std::cout << "[Name: " << level->getName() << "]";
            std::cout << "[Desc: " << level->getDesc() << "]";
            std::cout << "[Consumed Power: " << level->getConsumedPower() << "]";
            std::cout << "[Exit latency: " << level->getExitLatency() << "]";
            std::cout << "[Absolute Time: " << level->getAbsoluteTime() << "]";
            std::cout << "[Absolute Count: " << level->getAbsoluteCount() << "]";
            std::cout << "[Enableable: " << level->isEnableable() << "]";
            std::cout << "[Enabled: " << level->isEnabled() << "]";

            /** Setting all the CPU to this state. **/
            std::vector<mammut::topology::VirtualCore*> virtualCores = cpu->getVirtualCores();
            for(size_t j = 0; j < virtualCores.size(); j++){
                std::vector<mammut::topology::VirtualCoreIdleLevel*> tmpLevels = virtualCores.at(j)->getIdleLevels();
                for(size_t k = 0; k < tmpLevels.size(); k++){
                    if(k != i){
                        tmpLevels.at(k)->disable();
                    }
                }
            }

            mammut::energy::CounterCpu* counter = mammut.getInstanceEnergy()->getCounterCpu(cpu->getCpuId());
            counter->reset();
            sleep(levelTime);

            std::cout << "[CPU Power: " << counter->getJoulesCpu() << "]";
            std::cout << "[Cores Power: " << counter->getJoulesCores() << "]";
            if(counter->hasJoulesDram()){
                std::cout << "[DRAM Power: " << counter->getJoulesDram() << "]";
            }
            if(counter->hasJoulesGraphic()){
                std::cout << "[Graphic Power: " << counter->getJoulesGraphic() << "]";
            }
            std::cout << std::endl;

            for(size_t j = 0; j < virtualCores.size(); j++){
                std::vector<mammut::topology::VirtualCoreIdleLevel*> tmpLevels = virtualCores.at(j)->getIdleLevels();
                for(size_t k = 0; k < tmpLevels.size(); k++){
                    if(k != i){
                        tmpLevels.at(k)->enable();
                    }
                }
            }
        }
    }
}
