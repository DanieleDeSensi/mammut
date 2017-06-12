#include "./mammut.hpp"

namespace mammut{

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

}
