/*
 * mammut-server.cpp
 *
 * Created on: 10/11/2014
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

#include <mammut/communicator.hpp>
#include <mammut/communicator-tcp.hpp>
#include <mammut/module.hpp>
#include <mammut/utils.hpp>
#include <mammut/cpufreq/cpufreq.hpp>
#include <mammut/topology/topology.hpp>
#include <mammut/energy/energy.hpp>

#include <getopt.h>
#include <inttypes.h>
#include <iostream>
#include <list>
#include <map>
#include <stddef.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int verbose = 0;

#define TRACE(level, trace) do{if(verbose >= level){std::cout << trace << std::endl;}}while(0)

namespace mammut{

void printUsage(char* progName){
    std::cerr << std::endl;
    std::cerr << "Usage: " << progName << " [--verbose][level] [--tcpport] [--cpufreq] [--topology] [--energy]" << std::endl;
    std::cerr << "[--verbose][level] | Activates verbose logging, levels available [0,1,2]." << std::endl;
    std::cerr << "[--tcpport]        | TCP port used by the server to wait for remote requests." << std::endl;
    std::cerr << "[--cpufreq]        | Activates cpufreq module." << std::endl;
    std::cerr << "[--topology]       | Activates topology module." << std::endl;
    std::cerr << "[--energy]         | Activates energy module." << std::endl;
}

typedef struct{
    int cpufreq;
    int topology;
    int energy;
}ModulesMask;

#define MAMMUT_SERVER_CREATE_MODULE(moduleType) do{                                                                            \
                                                  try{                                                                       \
                                                      _moduleDispatcher[moduleType::getModuleName()] = moduleType::local();  \
                                                  }catch(...){                                                               \
                                                      std::cerr << "Impossible to create module " <<  #moduleType;           \
                                                  }                                                                          \
                                              }while(0)                                                                      \

#define MAMMUT_SERVER_DELETE_MODULE(moduleType) do{                                                                 \
                                                  std::map<std::string, Module*>::iterator it;                    \
                                                  it = _moduleDispatcher.find(moduleType::getModuleName());       \
                                                  if(it != _moduleDispatcher.end()){                              \
                                                      moduleType::release(dynamic_cast<moduleType*>(it->second)); \
                                                      _moduleDispatcher.erase(it);                                \
                                                  }                                                               \
                                              }while(0)                                                           \

class Servant: public utils::Thread{
private:
    Communicator* const _communicator;
    const ModulesMask& _mm;
    std::map<std::string, Module*> _moduleDispatcher;
public:
    Servant(const ServerTcp& serverTcp, const ModulesMask& mm):
        _communicator(new CommunicatorTcp(serverTcp)),
        _mm(mm){
        if(_mm.cpufreq){
            MAMMUT_SERVER_CREATE_MODULE(cpufreq::CpuFreq);
            TRACE(2, "CpuFreq module activated");
        }

        if(_mm.topology){
            MAMMUT_SERVER_CREATE_MODULE(topology::Topology);
            TRACE(2, "Topology module activated");
        }

        if(_mm.energy){
            MAMMUT_SERVER_CREATE_MODULE(energy::Energy);
            TRACE(2, "Energy module activated");
        }
    }

    ~Servant(){
        MAMMUT_SERVER_DELETE_MODULE(cpufreq::CpuFreq);
        MAMMUT_SERVER_DELETE_MODULE(topology::Topology);
        MAMMUT_SERVER_DELETE_MODULE(energy::Energy);
        delete _communicator;
    }

    void run(){
        std::string messageIdIn, messageIn, messageIdOut, messageOut;
        std::map<std::string, Module*>::const_iterator it;
        std::string moduleId;
        while(true){
            messageIdIn.clear();
            messageIn.clear();
            messageIdOut.clear();
            messageOut.clear();
            utils::ScopedLock(_communicator->getLock());

            try{
                TRACE(2, "Receiving message");
                if(!_communicator->receive(messageIdIn, messageIn)){
                    TRACE(2, "Connection closed");
                    return;
                }
                TRACE(2, "Received message: " + messageIdIn);
                moduleId = utils::getModuleNameFromMessageId(messageIdIn);
                TRACE(2, "From module: " + moduleId);
                it = _moduleDispatcher.find(moduleId);
                if(it == _moduleDispatcher.end() || it->second == NULL){
                    TRACE(2, "Module not activated");
                    throw std::runtime_error("Server: Module " + moduleId + " not activated.");
                }
                if(!it->second->processMessage(messageIdIn, messageIn, messageIdOut, messageOut)){
                    TRACE(2, "Error while processing message");
                    throw std::runtime_error("Server: Error while processing message " + messageIdIn + ".");
                }
                TRACE(2, "Sending response");
                _communicator->send(messageIdOut, messageOut);
            }catch(const std::runtime_error& exc){
                try{
                    TRACE(2, "Sending exception");
                    _communicator->send("", exc.what());
                }catch(...){
                    std::cerr << "FATAL communication error while communicating to the client." << std::endl;
                    return;
                }
            }catch(...){
                std::cerr <<  "FATAL processing error." << std::endl;
                return;
            }
        }
    }
};

class ServantsQueueCleaner: public utils::Thread{
private:
    std::list<Servant*>& _servants;
    utils::LockPthreadMutex& _lock;
public:
    ServantsQueueCleaner(std::list<Servant*>& servants, utils::LockPthreadMutex& lock):
        _servants(servants), _lock(lock){
        ;
    }

    void run(){
        while(true){
            TRACE(1, "Cleaning connections.");
            {
                utils::ScopedLock scopedLock(_lock);
                /** Checks if some of the started servants finished its execution. **/
                for(std::list<Servant*>::iterator it = _servants.begin(); it != _servants.end();){
                    if((*it)->finished()){
                        delete *it;
                        it = _servants.erase(it);
                        TRACE(1, "Connection cleaned.");
                    }else{
                        ++it;
                        TRACE(1, "Connection still active.");
                    }
                }
            }
            TRACE(1, "End of connections cleaning.");
            sleep(10);
        }
    }
};

bool checkDependencies(const ModulesMask& mm){
    if(mm.cpufreq && !mm.topology){
        std::cerr << "CpuFreq module needs topology module." << std::endl;
        return false;
    }

    if(mm.energy && !mm.topology){
        std::cerr << "Energy module needs topology module." << std::endl;
        return false;
    }

    return true;
}

}

int main(int argc, char** argv){
    mammut::ModulesMask mm;
    memset(&mm, 0, sizeof(mm));
    uint16_t tcpport = 0;
    static struct option long_options[] = {
        {"verbose",   required_argument, &verbose,       'v'},
        {"tcpport",   required_argument, 0,              'p'},
        {"cpufreq",   no_argument,       &(mm.cpufreq),  1},
        {"topology",  no_argument,       &(mm.topology), 1},
        {"energy",    no_argument,       &(mm.energy),   1},
        {0,           0,                 0,              0}
    };

    int long_index = 0;
    int opt = 0;
    while ((opt = getopt_long(argc, argv,"v:p:",
                   long_options, &long_index )) != -1) {
        switch (opt) {
            case 0:{
                /* If this option set a flag, do nothing else now. */
                if(long_options[long_index].flag != 0){
                    break;
                }else{
                    throw std::runtime_error("option " + std::string(long_options[long_index].name) +
                                             " with arg " + optarg + " without flags");
                }
            }break;
            case 'p':{
                tcpport = atoi(optarg);
            }break;
            case 'v':{
                verbose = atoi(optarg);
            }break;
            default:{
                mammut::printUsage(argv[0]);
                return -1;
            }
        }
    }

    if(!mammut::checkDependencies(mm)){
        return -1;
    }

    if(tcpport){
        mammut::ServerTcp tcpServer(tcpport);
        std::list<mammut::Servant*> servants;
        mammut::utils::LockPthreadMutex cleanerLock;
        mammut::ServantsQueueCleaner cleaner(servants, cleanerLock);
        cleaner.start();
        while(true){
            TRACE(1, "Waiting for new connection.");
            /** This is actually created only when and if a new connection request arrives. **/
            mammut::Servant* servant = new mammut::Servant(tcpServer, mm);
            servant->start();
            TRACE(1, "New connection estabilished.");

            {
                mammut::utils::ScopedLock scopedLock(cleanerLock);
                servants.push_back(servant);
            }
        }
    }else{
        mammut::printUsage(argv[0]);
        return -1;
    }
}
