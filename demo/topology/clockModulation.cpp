#include <mammut/mammut.hpp>

#include <cassert>
#include <iostream>
#include <unistd.h>

using namespace mammut;
using namespace mammut::topology;
using namespace std;

int main(int argc, char** argv){
    Mammut m;
    Topology* topology = m.getInstanceTopology();
    vector<Cpu*> cpus = topology->getCpus();
    if(argc < 2){
        std::cerr << "Usage: " << argv[0] << " ModulationValue(between 0 and 100)" << std::endl;
        std::cerr << "If modulation value is negative, the current modulation is read." << std::endl;
        return -1;
    }
    double value = atof(argv[1]);
    for(Cpu* c : cpus){
        if(value < 0){
            std::cout << "Modulation for one CPU: " << c->getClockModulation() << std::endl;
        }else{
            c->setClockModulation(value);
        }
    }
}
