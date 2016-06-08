#include <mammut/mammut.hpp>

#include <cassert>
#include <iostream>
#include <cmath>
#include <unistd.h>

using namespace mammut;
using namespace mammut::cpufreq;
using namespace std;

int main(int argc, char** argv){
    Mammut m;
    CpuFreq* frequency = m.getInstanceCpuFreq();

    /** Analyzing domains. **/
    vector<Domain*> domains = frequency->getDomains();
    cout << domains.size() << " frequency domains found" << endl;

    for(size_t i = 0; i < domains.size(); i++){
        Domain* domain = domains.at(i);
        vector<Frequency> frequencies = domain->getAvailableFrequencies();

        /** Set the domain to the lowest frequency. **/
        if(domain->isGovernorAvailable(GOVERNOR_USERSPACE) && frequencies.size()){
            Frequency target = frequencies.at(0);
            cout << "Setting domain " << domain->getId() << " to frequency " << target << endl;
            domain->setGovernor(GOVERNOR_USERSPACE);
            domain->setFrequencyUserspace(target);
        }
    }
}
