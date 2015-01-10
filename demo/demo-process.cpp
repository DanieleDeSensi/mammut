/*
 * demo-process.cpp
 * This is a minimal demo on processes and threads management.
 *
 * Created on: 10/01/2015
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

#include <mammut/process/process.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char** argv){
#if 0
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
#endif

    mammut::process::ProcessesManager* pm;
    pm = mammut::process::ProcessesManager::local();
    std::vector<mammut::process::Pid> processes = pm->getActiveProcessesIdentifiers();
    std::cout << "There are " << processes.size() << " active processes: ";
    for(size_t i = 0; i < processes.size(); i++){
        std::cout << "[" << processes.at(i) << "]";
    }
    std::cout << std::endl;

    std::cout << "Getting information on this process (Pid: " << getpid() << ")." << std::endl;
    mammut::process::ProcessHandler* thisProcess = pm->getProcessHandler(getpid());
    std::vector<mammut::process::Tid> threads = thisProcess->getActiveThreadsIdentifiers();
    std::cout << "There are " << threads.size() << " active threads on this process: ";
    for(size_t i = 0; i < threads.size(); i++){
        std::cout << "[" << threads.at(i) << "]";
    }
    std::cout << std::endl;

    double coreUsage = 0;
    uint sleepingSecs = 10;
    std::cout << "Sleeping " << sleepingSecs << " seconds." << std::endl;
    thisProcess->resetCoreUsage();
    sleep(sleepingSecs);
    thisProcess->getCoreUsage(coreUsage);
    std::cout << "Core usage over the last " << sleepingSecs << " seconds: " << coreUsage << std::endl;

    uint sinIterations = 100000000;
    double sinRes = rand();
    std::cout << "Computing " << sinIterations << " sin() iterations." << std::endl;
    thisProcess->resetCoreUsage();
    for(uint i = 0; i < sinIterations; i++){
        sinRes = sin(sinRes);
    }
    thisProcess->getCoreUsage(coreUsage);
    std::cout << "SinResult: " << sinRes << std::endl;
    std::cout << "Core usage during " << sinIterations << " sin iterations: " << coreUsage << std::endl;

    pm->releaseProcessHandler(thisProcess);
    mammut::process::ProcessesManager::release(pm);
    return 1;
}
