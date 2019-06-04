#include <mammut/energy/energy.hpp>
#include <mammut/energy/energy-linux.hpp>
#ifdef MAMMUT_REMOTE
#include <mammut/energy/energy-remote.hpp>
#include <mammut/energy/energy-remote.pb.h>
#endif
#include <mammut/topology/topology.hpp>

#include "stdexcept"

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

JoulesCpu CounterCpus::getJoulesComponents(topology::Cpu* cpu){
    return getJoulesComponents(cpu->getCpuId());
}

Joules CounterCpus::getJoulesCpu(topology::Cpu* cpu){
    return getJoulesCpu(cpu->getCpuId());
}

Joules CounterCpus::getJoulesCores(topology::Cpu* cpu){
    return getJoulesCores(cpu->getCpuId());
}

Joules CounterCpus::getJoulesGraphic(topology::Cpu* cpu){
    return getJoulesGraphic(cpu->getCpuId());
}

Joules CounterCpus::getJoulesDram(topology::Cpu* cpu){
    return getJoulesDram(cpu->getCpuId());
}

Energy::Energy(){
#if defined (__linux__)
    /******** Create plug counter (if present). ********/
    CounterPlugSmartGaugeLinux* cpl = new CounterPlugSmartGaugeLinux();
    if(cpl->init()){
        _counterPlug = cpl;
    }else{
        delete cpl;
        _counterPlug = NULL;
    }

    if(!_counterPlug){
        CounterPlugAmesterLinux* cpal = new CounterPlugAmesterLinux();
        if(cpal->init()){
            _counterPlug = cpal;
        }else{
            delete cpal;
            _counterPlug = NULL;
        }
    }

    /******** Create CPUs counter (if present). ********/
    CounterCpusLinux* ccl = new CounterCpusLinux();
    if(ccl->init()){
        _counterCpus = ccl;
    }else{
        delete ccl;
        _counterCpus = NULL;
    }

    /******** Create Memory counter (if present). ********/
    CounterMemoryRaplLinux* cml = new CounterMemoryRaplLinux();
    if(cml->init()){
        _counterMemory = cml;
    }else{
        delete cml;
        _counterMemory = NULL;
    }

    if(!_counterMemory){
        CounterMemoryAmesterLinux* cmal = new CounterMemoryAmesterLinux();
        if(cmal->init()){
            _counterMemory = cmal;
        }else{
            delete cmal;
            _counterMemory = NULL;
        }
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
        _counterPlug->reset();
    }else{
        delete cpl;
        _counterPlug = NULL;
    }

    /******** Create Memory counter (if present). ********/
    CounterMemoryRemote* cml = new CounterMemoryRemote(communicator);
    if(cml->init()){
        _counterMemory = cml;
    }else{
        delete cml;
        _counterMemory = NULL;
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

    if(_counterMemory){
        delete _counterMemory;
    }

    if(_counterCpus){
        delete _counterCpus;
    }
}

void Energy::release(Energy* energy){
    delete energy;
}

Counter* Energy::getCounter() const{
    std::vector<CounterType> cts = getCountersTypes();
    if(!cts.empty()){
        // Returned counters types are sorted from the most to the least precise.
        return getCounter(cts.at(0));
    }else{
        return NULL;
    }
}

std::vector<CounterType> Energy::getCountersTypes() const{
    std::vector<CounterType> r;
    // Set from the most precise to the least precise
    if(_counterCpus){
        r.push_back(COUNTER_CPUS);
    }

    if(_counterMemory){
        r.push_back(COUNTER_MEMORY);
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
        case COUNTER_MEMORY:{
            return _counterMemory;
        }break;
        case COUNTER_PLUG:{
            return _counterPlug;
        }break;
    }
    return NULL;
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
                case COUNTER_TYPE_PB_MEMORY:{
                    counter = _counterMemory;
                }break;
                case COUNTER_TYPE_PB_CPUS:{
                    counter = _counterCpus;
                }break;
            }
            switch(cr.cmd()){
                case COUNTER_COMMAND_INIT:{
                    CounterResBool cri;
                    if(counter){
                        cri.set_res(counter->init());
                    }else{
                        cri.set_res(false);
                    }
                    return utils::setMessageFromData(&cri, messageIdOut, messageOut);
                }break;
                case COUNTER_COMMAND_RESET:{
                    CounterResBool cri;
                    cri.set_res(true); // Anything is fine. Is ignored on the other side
                    counter->reset();
                    return utils::setMessageFromData(&cri, messageIdOut, messageOut);
                }break;
                case COUNTER_COMMAND_HAS:{
                    if(cr.type() == COUNTER_TYPE_PB_CPUS){
                        CounterResBool cri;
                        if(cr.subtype() == COUNTER_VALUE_TYPE_CORES){
                        	cri.set_res(_counterCpus->hasJoulesCores());
                        }else if(cr.subtype() == COUNTER_VALUE_TYPE_GRAPHIC){
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


