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
#include <mammut/energy/energy-remote.hpp>
#include <mammut/energy/energy-remote.pb.h>
#include <mammut/topology/topology.hpp>

#include <stdexcept>

namespace mammut{
namespace energy{

CounterCpu::CounterCpu(topology::Cpu* cpu, bool hasJoulesGraphic, bool hasJoulesDram):
        _cpu(cpu),
        _hasJoulesGraphic(hasJoulesGraphic),
        _hasJoulesDram(hasJoulesDram){
    ;
}

topology::Cpu* CounterCpu::getCpu(){
    return _cpu;
}

bool CounterCpu::hasJoulesGraphic(){
    return _hasJoulesGraphic;
}

bool CounterCpu::hasJoulesDram(){
    return _hasJoulesDram;
}

Energy::Energy():_topology(topology::Topology::local()){
#if defined (__linux__)
    std::vector<topology::Cpu*> cpus = _topology->getCpus();
    for(size_t i = 0; i < cpus.size(); i++){
        if(CounterCpuLinux::isCpuSupported(cpus.at(i))){
            _countersCpu.push_back(new CounterCpuLinux(cpus.at(i)));
        }
    }
#else
    throw new std::runtime_error("Energy: OS not supported.");
#endif
}

Energy* Energy::local(){
    return new Energy();
}

Energy::Energy(Communicator* const communicator):
         _topology(topology::Topology::remote(communicator)){
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

Energy::~Energy(){
    utils::deleteVectorElements<CounterCpu*>(_countersCpu);
    topology::Topology::release(_topology);
}

void Energy::release(Energy* energy){
    delete energy;
}

std::vector<CounterCpu*> Energy::getCountersCpu() const{
    return _countersCpu;
}

std::vector<CounterCpu*> Energy::getCountersCpu(const std::vector<topology::VirtualCore*>& virtualCores) const{
    std::vector<CounterCpu*> r;
    for(size_t i = 0; i < virtualCores.size(); i++){
        CounterCpu* currentCounter = getCounterCpu(virtualCores.at(i)->getCpuId());
        if(!utils::contains(r, currentCounter)){
            r.push_back(currentCounter);
        }
    }
    return r;
}

CounterCpu* Energy::getCounterCpu(topology::CpuId cpuId) const{
    CounterCpu* r = NULL;
    for(size_t i = 0; i < _countersCpu.size(); i++){
        r = _countersCpu.at(i);
        if(r->getCpu()->getCpuId() == cpuId){
            return r;
        }
    }

    return NULL;
}

void Energy::resetCountersCpu(){
    for(size_t i = 0; i < _countersCpu.size(); i++){
        _countersCpu.at(i)->reset();
    }
}

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
                	rSingle.set_joules(cc->getJoulesCpu());
                	r = &rSingle;
                }break;
                case COUNTER_CPU_TYPE_CORES:{
                	rSingle.set_joules(cc->getJoulesCores());
                	r = &rSingle;
                }break;
                case COUNTER_CPU_TYPE_GRAPHIC:{
                	rSingle.set_joules(cc->getJoulesGraphic());
                	r = &rSingle;
                }break;
                case COUNTER_CPU_TYPE_DRAM:{
                	rSingle.set_joules(cc->getJoulesDram());
                	r = &rSingle;
                }break;
            }
            return utils::setMessageFromData(r, messageIdOut, messageOut);
        }
    }

    return false;
}

}
}


