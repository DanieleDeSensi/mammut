/*
 * demo-energy-extended.cpp
 *
 * Created on: 04/12/2014
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

#include <mammut/communicator-tcp.hpp>
#include <mammut/energy/energy.hpp>

#include <iostream>
#include <unistd.h>

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

    mammut::energy::Energy* energy;

    /** A local or a remote handler is built. **/
    if(communicator){
        energy = mammut::energy::Energy::remote(communicator);
    }else{
        energy = mammut::energy::Energy::local();
    }

    /** Gets the energy counters (one per CPU). **/
    std::vector<mammut::energy::CounterCpu*> counters = energy->getCountersCpu();
    if(counters.size() == 0){
        std::cout << "Cpu counters not present on this machine." << std::endl;
        return 0;
    }

    /** Prints information for each counter. **/
    for(size_t i = 0; i < counters.size(); i++){
        mammut::energy::CounterCpu* c = counters.at(i);
        std::cout << "Found Cpu counter for cpu: " << c->getCpu()->getId() << " ";
        std::cout << "Has graphic counter: " << c->hasJoulesGraphic() << " ";
        std::cout << "Has Dram counter: " << c->hasJoulesDram() << " ";
        std::cout << std::endl;
    }

    /** Gets the value of the counter every sleepingSecs seconds. **/
    unsigned int sleepingSecs = 10;
    unsigned int iterations = 4;
    for(unsigned int i = 0; i < iterations; i++){
        std::cout << "Sleeping " << sleepingSecs << " seconds." << std::endl;
        sleep(sleepingSecs);
        for(size_t j = 0; j < counters.size(); j++){
            mammut::energy::CounterCpu* c = counters.at(j);
            std::cout << "Joules consumed for CPU " << c->getCpu()->getId() << " in the last " << sleepingSecs << " seconds: ";
            std::cout << "Cpu: " << c->getJoules() << " ";
            std::cout << "Cores: " << c->getJoulesCores() << " ";
            if(c->hasJoulesGraphic()){std::cout << "Graphic: " << c->getJoulesGraphic() << " ";}
            if(c->hasJoulesDram()){std::cout << "Dram: " << c->getJoulesDram() << " ";}
            std::cout << std::endl;
            c->reset();
        }
    }

    mammut::energy::Energy::release(energy);
}
