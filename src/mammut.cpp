#include <mammut/mammut.hpp>
#include <mammut/mammut.h>
#include <cstring>

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
using namespace mammut::topology;
using namespace mammut::cpufreq;


MammutHandle *createMammut(){
  return reinterpret_cast<MammutHandle*>(new Mammut());
}

void destroyMammut(MammutHandle *m){
  delete reinterpret_cast<Mammut*>(m);
}

/*** Class Energy **/

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

MammutEnergyJoules getJoulesCpu(MammutEnergyCounterCpus* c, MammutCpuHandle* cpu){
  return static_cast<MammutEnergyJoules>(reinterpret_cast<CounterCpus*>(c)->getJoulesCpu(reinterpret_cast<Cpu*>(cpu)));
}

MammutEnergyJoules getJoulesCoresAll(MammutEnergyCounterCpus* c){
  return static_cast<MammutEnergyJoules>(reinterpret_cast<CounterCpus*>(c)->getJoulesCoresAll());
}

MammutEnergyJoules getJoulesCores(MammutEnergyCounterCpus* c, MammutCpuHandle* cpu){
  return static_cast<MammutEnergyJoules>(reinterpret_cast<CounterCpus*>(c)->getJoulesCores(reinterpret_cast<Cpu*>(cpu)));
}

int hasJoulesGraphic(MammutEnergyCounterCpus* c){
  return static_cast<int>(reinterpret_cast<CounterCpus*>(c)->hasJoulesGraphic());
}

MammutEnergyJoules getJoulesGraphicAll(MammutEnergyCounterCpus* c){
  return static_cast<MammutEnergyJoules>(reinterpret_cast<CounterCpus*>(c)->getJoulesGraphicAll());
}

MammutEnergyJoules getJoulesGraphic(MammutEnergyCounterCpus* c, MammutCpuHandle* cpu){
  return static_cast<MammutEnergyJoules>(reinterpret_cast<CounterCpus*>(c)->getJoulesGraphic(reinterpret_cast<Cpu*>(cpu)));
}

int hasJoulesDram(MammutEnergyCounterCpus* c){
  return static_cast<int>(reinterpret_cast<CounterCpus*>(c)->hasJoulesDram());
}

MammutEnergyJoules getJoulesDramAll(MammutEnergyCounterCpus* c){
  return static_cast<MammutEnergyJoules>(reinterpret_cast<CounterCpus*>(c)->getJoulesDramAll());
}

MammutEnergyJoules getJoulesDram(MammutEnergyCounterCpus* c, MammutCpuHandle* cpu){
  return static_cast<MammutEnergyJoules>(reinterpret_cast<CounterCpus*>(c)->getJoulesDram(reinterpret_cast<Cpu*>(cpu)));
}

/** Class Topology **/

MammutTopologyHandle* getInstanceTopology(MammutHandle* m){
  return reinterpret_cast<MammutTopologyHandle*>(reinterpret_cast<Mammut*>(m)->getInstanceTopology());
}

void getCpus(MammutTopologyHandle* t, MammutCpuHandle*** cpus, size_t* cpusSize){
  std::vector<Cpu*> tcpus = reinterpret_cast<Topology*>(t)->getCpus();
  *cpusSize = tcpus.size() ;
  *cpus = reinterpret_cast<MammutCpuHandle**>(malloc(sizeof(MammutCpuHandle*)*tcpus.size()));
  for(size_t i = 0; i < *cpusSize; i++){
    (*cpus)[i] = reinterpret_cast<MammutCpuHandle*>(tcpus[i]);
  }
}

/** Class CPU */


/* warning because ocaml-ctypes doesn't support const qualifiers yet */
const char* getFamily(MammutCpuHandle* c){
  return reinterpret_cast<Cpu*>(c)->getFamily().c_str() ;
}

const char* getModel(MammutCpuHandle* c){
  return reinterpret_cast<Cpu*>(c)->getModel().c_str() ;
}

int getCpuId(MammutCpuHandle* c){
  return reinterpret_cast<Cpu*>(c)->getCpuId() ;
}



/** Class CpuFreq **/

MammutCpuFreqHandle* getInstanceCpuFreq(MammutHandle* m){
  return reinterpret_cast<MammutCpuFreqHandle*>(reinterpret_cast<Mammut*>(m)->getInstanceCpuFreq());
}


void disableBoosting(MammutCpuFreqHandle* cf){
  reinterpret_cast<CpuFreq*>(cf)->disableBoosting();
}


void enableBoosting(MammutCpuFreqHandle* cf){
  reinterpret_cast<CpuFreq*>(cf)->enableBoosting();
}


int isBoostingEnabled(MammutCpuFreqHandle* cf){
  return static_cast<int>(reinterpret_cast<CpuFreq*>(cf)->isBoostingEnabled());
}

int isBoostingSupported(MammutCpuFreqHandle* cf){
  return static_cast<int>(reinterpret_cast<CpuFreq*>(cf)->isBoostingSupported());
}
