#include <mammut/mammut.hpp>

#include <cassert>
#include <iostream>
#include <unistd.h>

using namespace mammut;
using namespace mammut::energy;
using namespace mammut::topology;
using namespace std;

int main(int argc, char** argv){
    Mammut m;
    Energy* energy = m.getInstanceEnergy();

    /** Gets the power plug counter. **/
    CounterPlug* counterPlug = dynamic_cast<CounterPlut*>(energy->getCounter(COUNTER_PLUG));
    if(!counterPlug){
        cout << "Plug counter not present on this machine." << endl;
    }

    /** Gets the CPUs energy counters. **/
    CounterCpus* counterCpus = dynamic_cast<CounterCpus*>(energy->getCounter(COUNTER_CPUS));
    if(!counterCpus){
        cout << "Cpu counters not present on this machine." << endl;
    }

    /** Prints information for each counter. **/
    if(counterCpus){
        cout << "Found Cpus counter ";
        cout << "Has cores counter: " << counterCpus->hasJoulesCores() << " ";
        cout << "Has graphic counter: " << counterCpus->hasJoulesGraphic() << " ";
        cout << "Has Dram counter: " << counterCpus->hasJoulesDram() << " ";
        cout << endl;
    }

    /** Gets the value of the counter every sleepingSecs seconds. **/
    unsigned int sleepingSecs = 10;
    unsigned int iterations = 4;
    for(unsigned int i = 0; i < iterations; i++){
        cout << "Sleeping " << sleepingSecs << " seconds." << endl;
        sleep(sleepingSecs);
        if(counterPlug){
            cout << "Joules consumed at power plug: " << counterPlug->getJoules() << endl;
            counterPlug->reset();
        }

	if(counterCpus){
            vector<Cpu*> cpus = m.getInstanceTopology()->getCpus();
            for(size_t j = 0; j < cpus.size(); j++){
                CpuId id = cpus.at(j)->getCpuId();
                cout << "Joules consumed for CPU " << id << ": ";
                cout << "Cpu: " << counterCpus->getJoulesCpu(id) << " ";
                if(counterCpus->hasJoulesCores()){
                	cout << "Cores: " << counterCpus->getJoulesCores(id) << " ";
                }
                if(counterCpus->hasJoulesGraphic()){
                    cout << "Graphic: " << counterCpus->getJoulesGraphic(id) << " ";
                }
                if(counterCpus->hasJoulesDram()){
                    cout << "Dram: " << counterCpus->getJoulesDram(id) << " ";
                }
                cout << endl;
            }
            counterCpus->reset();
	}
    }
}
