#include <mammut/mammut.hpp>

#include <cassert>
#include <iostream>
#include <unistd.h>

double seconds = 20;

using namespace mammut;
using namespace mammut::cpufreq;
using namespace mammut::energy;
using namespace mammut::topology;
using namespace std;

int main(int argc, char** argv){
    if(argc<2){
        cerr << "Usage: " << argv[0] << " idlePower" << endl;
    }
    double idlePower = atof(argv[1]);
    Mammut mammut;
    Energy* energy = mammut.getInstanceEnergy();
    CpuFreq* frequency = mammut.getInstanceCpuFreq();
    vector<Domain*> domains = frequency->getDomains();
    assert(domains.size());
    Domain* domain = domains.at(0);
    VirtualCore* vc = domain->getVirtualCores().at(0);
    vector<Frequency> availableFrequencies = domain->getAvailableFrequencies();
    CounterCpus* counter = dynamic_cast<CounterCpus*>(energy->getCounter(COUNTER_CPUS));
    vector<PhysicalCore*> physicalCores = mammut.getInstanceTopology()->virtualToPhysical(domain->getVirtualCores());

    cpufreq::RollbackPoint rp = frequency->getRollbackPoint();
    domain->setGovernor(GOVERNOR_USERSPACE);

    for(size_t i = 0; i < availableFrequencies.size(); i++){
        domain->setFrequencyUserspace(availableFrequencies.at(i));
        cout << "data = {";
        for(size_t j = 0; j < physicalCores.size(); j++){
            cout << "{" << j+1 << ",";
            for(size_t k = 0; k <= j; k++){
                physicalCores.at(k)->getVirtualCore()->maximizeUtilization();
            }
            counter->reset();
            sleep(seconds);
            cout << counter->getJoulesCores(vc->getCpuId())/seconds - idlePower<< "}";
            if(j < physicalCores.size() - 1){
                cout << ",";
            }
            for(size_t k = 0; k <= j; k++){
                physicalCores.at(k)->getVirtualCore()->resetUtilization();
            }
        }
        cout << "}" << endl;
        cout << "lm = LinearModelFit[data, x, x]" << endl;
    }

    frequency->rollback(rp);
}
