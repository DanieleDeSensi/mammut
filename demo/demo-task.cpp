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

#include <mammut/communicator-tcp.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "../mammut/task/task.hpp"

static pid_t gettid(){
#ifdef SYS_gettid
    return syscall(SYS_gettid);
#else
    throw std::runtime_error("gettid() not available.");
#endif
}

void* idleThread(void* arg){
    pid_t tid = gettid();
    mammut::task::ThreadHandler* thisThread = ((mammut::task::ProcessHandler*) arg)->getThreadHandler(tid);
    double coreUsage = 0;
    uint sleepingSecs = 10;
    std::cout << "[Idle Thread] Created [Tid: " << tid << "]. Sleeping " << sleepingSecs << " seconds." << std::endl;
    thisThread->resetCoreUsage();
    sleep(sleepingSecs);
    thisThread->getCoreUsage(coreUsage);
    std::cout << "[Idle Thread] Core usage over the last " << sleepingSecs << " seconds: " << coreUsage << "%" << std::endl;
    ((mammut::task::ProcessHandler*) arg)->releaseThreadHandler(thisThread);
    return NULL;
}

void* sinThread(void* arg){
    pid_t tid = gettid();
    mammut::task::ThreadHandler* thisThread = ((mammut::task::ProcessHandler*) arg)->getThreadHandler(tid);
    double coreUsage = 0;
    uint sinIterations = 100000000;
    double sinRes = rand();
    std::cout << "[Sin Thread] Created [Tid: " << tid << "]. Computing " << sinIterations << " sin() iterations." << std::endl;
    thisThread->resetCoreUsage();
    for(uint i = 0; i < sinIterations; i++){
        sinRes = sin(sinRes);
    }
    thisThread->getCoreUsage(coreUsage);
    std::cout << "[Sin Thread] SinResult: " << sinRes << std::endl;
    std::cout << "[Sin Thread] Core usage during " << sinIterations << " sin iterations: " << coreUsage << "%" << std::endl;
    ((mammut::task::ProcessHandler*) arg)->releaseThreadHandler(thisThread);
    return NULL;
}

int main(int argc, char** argv){
    mammut::CommunicatorTcp* communicator = NULL;
#if 0
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

    mammut::topology::Topology* topology;
    mammut::task::TasksManager* pm;
    if(communicator){
        //pm = mammut::process::ProcessesManager::remote(communicator);
        topology = mammut::topology::Topology::remote(communicator);
    }else{
        pm = mammut::task::TasksManager::local();
        topology = mammut::topology::Topology::local();
    }
    std::vector<mammut::task::TaskId> processes = pm->getActiveProcessesIdentifiers();
    std::cout << "There are " << processes.size() << " active processes: ";
    for(size_t i = 0; i < processes.size(); i++){
        std::cout << "[" << processes.at(i) << "]";
    }
    std::cout << std::endl;

    std::cout << "[Process] Getting information (Pid: " << getpid() << ")." << std::endl;
    mammut::task::ProcessHandler* thisProcess = pm->getProcessHandler(getpid());

    mammut::topology::VirtualCoreId vid;
    assert(thisProcess->getVirtualCoreId(vid));
    std::vector<mammut::topology::PhysicalCore*> physicalCores = topology->getPhysicalCores();
    std::cout << "[Process] Currently running on virtual core: " << vid << std::endl;
    std::cout << "[Process] Moving to physical core " << physicalCores.back()->getPhysicalCoreId() << std::endl;
    //    assert(thisProcess->move(physicalCores.back()));

    std::cout << "[Process] Creating some threads..." << std::endl;
    pthread_t tid_1, tid_2;
    pthread_create(&tid_1, NULL, idleThread, thisProcess);
    pthread_create(&tid_2, NULL, sinThread, thisProcess);

    std::vector<mammut::task::TaskId> threads = thisProcess->getActiveThreadsIdentifiers();
    std::cout << "[Process] There are " << threads.size() << " active threads: ";
    for(size_t i = 0; i < threads.size(); i++){
        std::cout << "[" << threads.at(i) << "]";
    }
    std::cout << std::endl;

    uint priority;
    assert(thisProcess->getPriority(priority));
    std::cout << "[Process] Current priority: " << priority << std::endl;
    std::cout << "[Process] Try to change priority to max (" << MAMMUT_PROCESS_PRIORITY_MAX << ")" << std::endl;
    assert(thisProcess->setPriority(MAMMUT_PROCESS_PRIORITY_MAX));
    sleep(1);
    assert(thisProcess->getPriority(priority));
    std::cout << "[Process] New priority: " << priority << std::endl;
    assert(priority == MAMMUT_PROCESS_PRIORITY_MAX);

    double coreUsage = 0;
    thisProcess->resetCoreUsage();
    pthread_join(tid_1, NULL);
    pthread_join(tid_2, NULL);
    thisProcess->getCoreUsage(coreUsage);
    std::cout << "[Process] Core usage " << coreUsage << "%" << std::endl;

    pm->releaseProcessHandler(thisProcess);
    mammut::task::TasksManager::release(pm);
    mammut::topology::Topology::release(topology);
    return 1;
}
