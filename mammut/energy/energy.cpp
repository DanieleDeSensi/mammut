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

JoulesCpu CounterCpus::getValuesAll(){
    JoulesCpu r;
    for(size_t i = 0; i < _cpus.size(); i++){
        r += getValues(_cpus.at(i)->getCpuId());
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

Joules CounterCpus::getValue(){
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
Energy::Energy(Communicator* const communicator):
         _topology(topology::Topology::remote(communicator)){
    _counterPlug = NULL; //TODO: Implement remote support for plug counters
    CountersCpuGet gcc;
    CountersCpuGetRes r;
    communicator->remoteCall(gcc, r);
    CountersCpuGetRes_CounterCpu counter;
    for(size_t i = 0; i < (size_t) r.counters_size(); i++){
         counter = r.counters(i);
        _countersCpu.push_back(new CounterCpuRemote(communicator,
                                                    _topology->getCpu(counter.cpu_id()),
                                                    counter.has_graphic(),
                                                    counter.has_dram()));
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
    CountersCpuGet ccg;
    return utils::getModuleNameFromMessage(&ccg);
}


bool Energy::processMessage(const std::string& messageIdIn, const std::string& messageIn,
                             std::string& messageIdOut, std::string& messageOut){
    {
        CountersCpuGet ccg;
        if(utils::getDataFromMessage<CountersCpuGet>(messageIdIn, messageIn, ccg)){
            CountersCpuGetRes r;
            for(size_t i = 0; i < _countersCpu.size(); i++){
                CounterCpu* cc = _countersCpu.at(i);
                CountersCpuGetRes_CounterCpu* outCounter = r.mutable_counters()->Add();
                outCounter->set_cpu_id(cc->getCpu()->getCpuId());
                outCounter->set_has_graphic(cc->hasJoulesGraphic());
                outCounter->set_has_dram(cc->hasJoulesDram());
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        CounterCpuReset ccr;
        if(utils::getDataFromMessage<CounterCpuReset>(messageIdIn, messageIn, ccr)){
            CounterCpuResetRes r;
            CounterCpu* cc = getCounterCpu(ccr.cpu_id());
            cc->reset();
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        CounterCpuGetJoules ccgj;
        if(utils::getDataFromMessage<CounterCpuGetJoules>(messageIdIn, messageIn, ccgj)){
            CounterCpu* cc = getCounterCpu(ccgj.cpu_id());
            ::google::protobuf::MessageLite* r = NULL;
            CounterCpuGetJoulesAllRes rAll;
            CounterCpuGetJoulesRes rSingle;
            switch(ccgj.type()){
				case COUNTER_CPU_TYPE_ALL:{
					JoulesCpu jc = cc->getJoules();
					rAll.set_joules_cpu(jc.cpu);
					rAll.set_joules_cores(jc.cores);
					rAll.set_joules_graphic(jc.graphic);
					rAll.set_joules_dram(jc.dram);
					r = &rAll;
				}break;
                case COUNTER_CPU_TYPE_CPU:{
                	rSingle.set_joules(cc->getJoulesCpuAll());
                	r = &rSingle;
                }break;
                case COUNTER_CPU_TYPE_CORES:{
                	rSingle.set_joules(cc->getJoulesCoresAll());
                	r = &rSingle;
                }break;
                case COUNTER_CPU_TYPE_GRAPHIC:{
                	rSingle.set_joules(cc->getJoulesGraphicAll());
                	r = &rSingle;
                }break;
                case COUNTER_CPU_TYPE_DRAM:{
                	rSingle.set_joules(cc->getJoulesDramAll());
                	r = &rSingle;
                }break;
            }
            return utils::setMessageFromData(r, messageIdOut, messageOut);
        }
    }

    return false;
}
#endif

}
}


