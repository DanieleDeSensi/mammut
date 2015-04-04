/*
 * demo-frequency.cpp
 *
 * Created on: 15/01/2015
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
#include <mammut/cpufreq/cpufreq.hpp>

#include <iostream>
#include <cmath>
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

    mammut::cpufreq::CpuFreq* frequency;

    /** A local or a remote handler is built. **/
    if(communicator){
        frequency = mammut::cpufreq::CpuFreq::remote(communicator);
    }else{
        frequency = mammut::cpufreq::CpuFreq::local();
    }


    mammut::cpufreq::Domain* domain = frequency->getDomains().at(0);
    std::cout << "Starting computing the voltage table..." << std::endl;
    mammut::cpufreq::VoltageTable vt = domain->getVoltageTable();
    mammut::cpufreq::dumpVoltageTable(vt, "voltageTable.txt");
    std::cout << "Voltage table computed and dumped on file" << std::endl;

    mammut::cpufreq::CpuFreq::release(frequency);
}
