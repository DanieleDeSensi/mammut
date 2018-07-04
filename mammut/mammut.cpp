#include "./mammut.hpp"
#include "./mammut.h"

namespace mammut{

SimulationParameters simulationParameters;

Mammut::Mammut(Communicator* const communicator):_cpufreq(NULL), _energy(NULL),
                                                 _topology(NULL), _task(NULL),
                                                 _communicator(communicator){
    ;
}

Mammut::Mammut(const Mammut& other):_cpufreq(NULL), _energy(NULL),
                                    _topology(NULL), _task(NULL){
    this->_communicator = other._communicator;
}

Mammut::~Mammut(){
    releaseModules();
}

void Mammut::releaseModules(){
    if(_cpufreq){
        cpufreq::CpuFreq::release(_cpufreq);
        _cpufreq = NULL;
    }

    if(_energy){
        energy::Energy::release(_energy);
        _energy = NULL;
    }

    if(_task){
        task::TasksManager::release(_task);
        _task = NULL;
    }

    if(_topology){
        topology::Topology::release(_topology);
        _topology = NULL;
    }
}

cpufreq::CpuFreq* Mammut::getInstanceCpuFreq() const{
    if(!_cpufreq){
        _cpufreq = cpufreq::CpuFreq::getInstance(_communicator);
    }
    return _cpufreq;
}

energy::Energy* Mammut::getInstanceEnergy() const{
    if(!_energy){
        _energy = energy::Energy::getInstance(_communicator);
    }
    return _energy;
}

task::TasksManager* Mammut::getInstanceTask() const{
    if(!_task){
        _task = task::TasksManager::getInstance(_communicator);
    }
    return _task;
}

topology::Topology* Mammut::getInstanceTopology() const{
    if(!_topology){
        _topology = topology::Topology::getInstance(_communicator);
    }
    return _topology;
}

Mammut& Mammut::operator=(const Mammut& other){
    if(this != &other){
        releaseModules();
        this->_communicator = other._communicator;
    }
    return *this;
}

void Mammut::setSimulationParameters(SimulationParameters& p){
    simulationParameters = p;
}

}

/**** C Interface implementation ****/
using namespace mammut;
using namespace mammut::energy;

MammutHandle *createMammut(){
    return reinterpret_cast<MammutHandle*>(new Mammut());
}

void destroyMammut(MammutHandle *m){
    delete reinterpret_cast<Mammut*>(m);
}

MammutEnergyHandle* getInstanceEnergy(MammutHandle* m){
    return reinterpret_cast<MammutEnergyHandle*>(reinterpret_cast<Mammut*>(m)->getInstanceEnergy());
}

MammutEnergyCounter* getCounter(MammutEnergyHandle* e){
    return reinterpret_cast<MammutEnergyCounter*>(reinterpret_cast<Energy*>(e)->getCounter());
}

MammutEnergyCounter* getCounterByType(MammutEnergyHandle* e, MammutEnergyCounterType ct){
    return reinterpret_cast<MammutEnergyCounter*>(reinterpret_cast<Energy*>(e)->getCounter(static_cast<CounterType>(ct)));
}

void getCountersTypes(MammutEnergyHandle* e, MammutEnergyCounterType** cts, size_t* ctsSize){
    std::vector<CounterType> types = reinterpret_cast<Energy*>(e)->getCountersTypes();
    *ctsSize = types.size();
    *cts = reinterpret_cast<MammutEnergyCounterType*>(malloc(sizeof(MammutEnergyCounterType)*types.size()));
    for(size_t i = 0; i < *ctsSize; i++){
        (*cts)[i] = static_cast<MammutEnergyCounterType>(types[i]);
    }
}

/** Class Counter */
MammutEnergyJoules getJoules(MammutEnergyCounter* c){
    return static_cast<MammutEnergyJoules>(reinterpret_cast<Counter*>(c)->getJoules());
}

void reset(MammutEnergyCounter* c){
    reinterpret_cast<Counter*>(c)->reset();
}

MammutEnergyCounterType getType(MammutEnergyCounter* c){
    return static_cast<MammutEnergyCounterType>(reinterpret_cast<Counter*>(c)->getType());
}

/** Class CounterCpus */
MammutEnergyJoules getJoulesCpuAll(MammutEnergyCounterCpus* c){
    return static_cast<MammutEnergyJoules>(reinterpret_cast<CounterCpus*>(c)->getJoulesCpuAll());
}

MammutEnergyJoules getJoulesCoresAll(MammutEnergyCounterCpus* c){
    return static_cast<MammutEnergyJoules>(reinterpret_cast<CounterCpus*>(c)->getJoulesCoresAll());
}

int hasJoulesGraphic(MammutEnergyCounterCpus* c){
    return static_cast<int>(reinterpret_cast<CounterCpus*>(c)->hasJoulesGraphic());
}

MammutEnergyJoules getJoulesGraphicAll(MammutEnergyCounterCpus* c){
    return static_cast<MammutEnergyJoules>(reinterpret_cast<CounterCpus*>(c)->getJoulesGraphicAll());
}

int hasJoulesDram(MammutEnergyCounterCpus* c){
    return static_cast<int>(reinterpret_cast<CounterCpus*>(c)->hasJoulesDram());
}

MammutEnergyJoules getJoulesDramAll(MammutEnergyCounterCpus* c){
    return static_cast<MammutEnergyJoules>(reinterpret_cast<CounterCpus*>(c)->getJoulesDramAll());
}