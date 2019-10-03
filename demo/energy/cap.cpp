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

    if(argc != 2){
      cerr << "Usage " << argv[0] << " powerCap";
      return -1;
    }

    PowerCapper* pcCpus = energy->getPowerCapper(COUNTER_CPUS);
    if(!pcCpus){
        cerr << "Power capper for CPUs not available on this machine." << endl;
        return -1;
    }

    PowerCap p;
    p.value = atoi(argv[1]);
    p.window = 1;
    pcCpus->set(p);

    /*
    PowerCapper* pcMem = energy->getPowerCapper(COUNTER_MEMORY);
    if(!pcMem){
        cout << "Power capper for DRAM not available on this machine." << endl;
    }

    PowerCapper* pcSys = energy->getPowerCapper(COUNTER_PLUG);
    if(!pcSys){
        cout << "Power capper for System not available on this machine." << endl;
    }
    */
}
