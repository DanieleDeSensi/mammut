/*
 * energy.cpp
 *
 * Created on: 01/12/2014
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

#include <mammut/energy/energy.hpp>
#include <mammut/energy/energy-linux.hpp>
#ifdef MAMMUT_REMOTE
#include <mammut/energy/energy-remote.hpp>
#include <mammut/energy/energy-remote.pb.h>
#endif
#include <mammut/topology/topology.hpp>

#include <stdexcept>

namespace mammut{
namespace energy{

CounterCpus::CounterCpus(topology::Topology* topology):
        _topology(topology),
        _cpus(_topology->getCpus()){
    ;
}

JoulesCpu CounterCpus::getJoulesComponentsAll(){
    JoulesCpu r;
    for(size_t i = 0; i < _cpus.size(); i++){
        r += getJoulesComponents(_cpus.at(i)->getCpuId());
    }
    return r;
}

Joules CounterCpus::getJoulesCpuAll(){
    Joules r = 0;
    for(size_t i = 0; i < _cpus.size(); i++){
        r += getJoulesCpu(_cpus.at(i)->getCpuId());
    }
    return r;
}

Joules CounterCpus::getJoulesCoresAll(){
    Joules r = 0;
    for(size_t i = 0; i < _cpus.size(); i++){
        r += getJoulesCores(_cpus.at(i)->getCpuId());
    }
    return r;
}

Joules CounterCpus::getJoulesGraphicAll(){
    Joules r = 0;
    for(size_t i = 0; i < _cpus.size(); i++){
        r += getJoulesGraphic(_cpus.at(i)->getCpuId());
    }
    return r;
}

Joules CounterCpus::getJoulesDramAll(){
    Joules r = 0;
    for(size_t i = 0; i < _cpus.size(); i++){
        r += getJoulesDram(_cpus.at(i)->getCpuId());
    }
    return r;
}

Joules CounterCpus::getJoules(){
    return getJoulesCpuAll();
}

Energy::Energy(){
#if defined (__linux__)
    /******** Create plug counter (if present). ********/
    CounterPlugLinux* cpl = new CounterPlugLinux();
    if(cpl->init()){
        _counterPlug = cpl;
    }else{
        delete cpl;
        _counterPlug = NULL;
    }

    /******** Create CPUs counter (if present). ********/
    CounterCpusLinux* ccl = new CounterCpusLinux();
    if(ccl->init()){
        _counterCpus = ccl;
    }else{
        delete ccl;
        _counterCpus = NULL;
    }
#else
    throw new std::runtime_error("Energy: OS not supported.");
#endif
}

Energy* Energy::local(){
    return new Energy();
}

#ifdef MAMMUT_REMOTE
Energy::Energy(Communicator* const communicator){
    /******** Create plug counter (if present). ********/
    CounterPlugRemote* cpl = new CounterPlugRemote(communicator);
    if(cpl->init()){
        _counterPlug = cpl;
    }else{
        delete cpl;
        _counterPlug = NULL;
    }

    /******** Create CPUs counter (if present). ********/
    CounterCpusRemote* ccl = new CounterCpusRemote(communicator);
    if(ccl->init()){
        _counterCpus = ccl;
    }else{
        delete ccl;
        _counterCpus = NULL;
    }
}

Energy* Energy::remote(Communicator* const communicator){
    return new Energy(communicator);
}
#else
Energy* Energy::remote(Communicator* const communicator){
    throw std::runtime_error("You need to define MAMMUT_REMOTE macro to use "
                             "remote capabilities.");
}
#endif

Energy::~Energy(){
    if(_counterPlug){
        delete _counterPlug;
    }

    if(_counterCpus){
        delete _counterCpus;
    }
}

void Energy::release(Energy* energy){
    delete energy;
}

std::vector<CounterType> Energy::getCountersTypes() const{
    std::vector<CounterType> r;
    if(_counterCpus){
        r.push_back(COUNTER_CPUS);
    }

    if(_counterPlug){
        r.push_back(COUNTER_PLUG);
    }

    return r;
}

Counter* Energy::getCounter(CounterType type) const{
    switch(type){
        case COUNTER_CPUS:{
            return _counterCpus;
        }break;
        case COUNTER_PLUG:{
            return _counterPlug;
        }break;
    }
    return NULL;
}

CounterPlug* Energy::getCounterPlug() const{
    return _counterPlug;
}

CounterCpus* Energy::getCounterCpus() const{
    return _counterCpus;
}

#ifdef MAMMUT_REMOTE
std::string Energy::getModuleName(){
    CounterReq cr;
    return utils::getModuleNameFromMessage(&cr);
}


bool Energy::processMessage(const std::string& messageIdIn, const std::string& messageIn,
                             std::string& messageIdOut, std::string& messageOut){
    {
        CounterReq cr;
        if(utils::getDataFromMessage<CounterReq>(messageIdIn, messageIn, cr)){
            Counter* counter = NULL;
            switch(cr.type()){
                case COUNTER_TYPE_PB_PLUG:{
                    counter = _counterPlug;
                }break;
                case COUNTER_TYPE_PB_CPUS:{
                    counter = _counterCpus;
                }break;
            }
            switch(cr.cmd()){
                case COUNTER_COMMAND_INIT:{
                    CounterResBool cri;
                    cri.set_res(counter->init());
                    return utils::setMessageFromData(&cri, messageIdOut, messageOut);
                }break;
                case COUNTER_COMMAND_RESET:{
                    CounterResBool cri;
                    counter->reset();
                    return utils::setMessageFromData(&cri, messageIdOut, messageOut);
                }break;
                case COUNTER_COMMAND_HAS:{
                    if(cr.type() == COUNTER_TYPE_PB_CPUS){
                        CounterResBool cri;
                        if(cr.subtype() == COUNTER_VALUE_TYPE_GRAPHIC){
                            cri.set_res(_counterCpus->hasJoulesGraphic());
                        }else if(cr.subtype() == COUNTER_VALUE_TYPE_DRAM){
                            cri.set_res(_counterCpus->hasJoulesDram());
                        }
                        return utils::setMessageFromData(&cri, messageIdOut, messageOut);
                    }
                }break;
                case COUNTER_COMMAND_GET:{
                    if(cr.type() == COUNTER_TYPE_PB_CPUS){
                        CounterResGetCpu crgc;
                        std::vector<topology::Cpu*> cpus = _counterCpus->getCpus();
                        for(size_t i = 0; i < cpus.size(); i++){
                            topology::Cpu* currentCpu = cpus.at(i);
                            JoulesCpu jc = _counterCpus->getJoulesComponents(currentCpu->getCpuId());

                            crgc.add_joules();
                            crgc.mutable_joules(i)->set_cpuid(currentCpu->getCpuId());
                            crgc.mutable_joules(i)->set_cpu(jc.cpu);
                            crgc.mutable_joules(i)->set_cores(jc.cores);
                            crgc.mutable_joules(i)->set_graphic(jc.graphic);
                            crgc.mutable_joules(i)->set_dram(jc.dram);
                        }

                        return utils::setMessageFromData(&crgc, messageIdOut, messageOut);
                    }else{
                        CounterResGetGeneric crgg;
                        crgg.set_joules(counter->getJoules());
                        return utils::setMessageFromData(&crgg, messageIdOut, messageOut);
                    }

                }break;
            }
        }
    }

    return false;
}
#endif

}
}


