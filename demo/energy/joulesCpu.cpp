#include <mammut/mammut.hpp>

#include <cassert>
#include <iostream>
#include <unistd.h>

using namespace mammut;
using namespace mammut::energy;
using namespace std;

int main(int argc, char** argv){
    Mammut m;
    Energy* energy = m.getInstanceEnergy();

    /** Gets the energy counters (one per CPU). **/
    CounterCpus* counterCpus = (CounterCpus*) energy->getCounter(COUNTER_CPUS);
    if(!counterCpus){
        cout << "Cpu counters not present on this machine." << endl;
        return -1;
    }

    cout << "Sleeping 10 seconds." << endl;    
    sleep(10);

    cout << "Total Cpus Joules: " << counterCpus->getJoulesCpuAll() << " ";
    if(counterCpus->hasJoulesCores()){
    	cout << "Total Cores Joules: " << counterCpus->getJoulesCoresAll() << " ";
    }
    if(counterCpus->hasJoulesDram()){
        cout << "Total Dram Joules: " << counterCpus->getJoulesDramAll() << " ";
    }
    if(counterCpus->hasJoulesGraphic()){
        cout << "Total Graphic Joules: " << counterCpus->getJoulesGraphicAll() << " ";
    }
    cout << endl;
}
