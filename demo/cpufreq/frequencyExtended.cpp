#include <mammut/mammut.hpp>

#include <cassert>
#include <iostream>
#include <cmath>
#include <unistd.h>

using namespace mammut;
using namespace mammut::cpufreq;

int main(int argc, char** argv){
    Mammut m;
    CpuFreq* frequency = m.getInstanceCpuFreq();

    /** Checks boosting support. **/
    if(!frequency->isBoostingSupported()){
        std::cout << "[Boosting not supported]" << std::endl;
    }else{
        if(frequency->isBoostingEnabled()){
            std::cout << "[Boosting enabled]" << std::endl;
            frequency->disableBoosting();
            assert(!frequency->isBoostingEnabled());
            frequency->enableBoosting();
            assert(frequency->isBoostingEnabled());
        }else{
            std::cout << "[Boosting disabled]" << std::endl;
            frequency->enableBoosting();
            assert(frequency->isBoostingEnabled());
            frequency->disableBoosting();
            assert(!frequency->isBoostingEnabled());
        }
        std::cout << "[Boosting enable/disable test passed]" << std::endl;
    }

    /** Analyzing domains. **/
    std::vector<Domain*> domains = frequency->getDomains();
    std::cout << "[" << domains.size() << " frequency domains found]" << std::endl;
    for(size_t i = 0; i < domains.size(); i++){
        bool userspaceAvailable = false;
        Domain* domain = domains.at(i);
        std::vector<mammut::topology::VirtualCoreId> identifiers = domain->getVirtualCoresIdentifiers();

        std::cout << "[Domain " << domain->getId() << "]";
        std::cout << "[Virtual Cores: ";
        for(size_t j = 0; j < identifiers.size(); j++){
            std::cout << identifiers.at(j) << ", ";
        }
        std::cout << "]" << std::endl;

        std::cout << "\tAvailable Governors: [";
        std::vector<Governor> governors = domain->getAvailableGovernors();
        for(size_t j = 0; j < governors.size() ; j++){
            if(governors.at(j) == GOVERNOR_USERSPACE){
                userspaceAvailable = true;
            }
            std::cout << frequency->getGovernorNameFromGovernor(governors.at(j)) << ", ";
        }
        std::cout << "]" << std::endl;
        std::cout << "\tAvailable Frequencies: [";
        std::vector<Frequency> frequencies = domain->getAvailableFrequencies();
        for(size_t j = 0; j < frequencies.size() ; j++){
            std::cout << frequencies.at(j) << "KHz, ";
        }
        std::cout << "]" << std::endl;

        std::cout << "\tTransition latency: " << domain->getTransitionLatency() << "ns." << std::endl;

        Governor currentGovernor = domain->getCurrentGovernor();
        std::cout << "\tCurrent Governor: [" << frequency->getGovernorNameFromGovernor(currentGovernor) << "]" << std::endl;
        Frequency lb, ub, currentFrequency;
        domain->getHardwareFrequencyBounds(lb, ub);
        std::cout << "\tHardware Frequency Bounds: [" << lb << "KHz, " << ub << "KHz]" << std::endl;
        if(currentGovernor == GOVERNOR_USERSPACE){
            currentFrequency = domain->getCurrentFrequencyUserspace();
        }else{
            currentFrequency = domain->getCurrentFrequency();
            assert(domain->getCurrentGovernorBounds(lb, ub));
            std::cout << "\tCurrent Governor Bounds: [" << lb << "KHz, " << ub << "KHz]" << std::endl;
        }
        std::cout << "\tCurrent Frequency: [" << currentFrequency << "]" << std::endl;

        /** Change frequency test. **/
        if(userspaceAvailable && frequencies.size()){
            domain->setGovernor(GOVERNOR_USERSPACE);
            assert(domain->getCurrentGovernor() == GOVERNOR_USERSPACE);
            domain->setFrequencyUserspace(frequencies.at(0));
            assert(domain->getCurrentFrequencyUserspace() == frequencies.at(0));
            domain->setHighestFrequencyUserspace();
            domain->setFrequencyUserspace(frequencies.back());
            assert(domain->getCurrentFrequencyUserspace() == frequencies.at(frequencies.size() - 1));
            /** Restore original governor and frequency. **/
            domain->setGovernor(currentGovernor);
            assert(currentGovernor == currentGovernor);
            if(currentGovernor == GOVERNOR_USERSPACE){
                domain->setFrequencyUserspace(currentFrequency);
                assert(domain->getCurrentFrequencyUserspace() == currentFrequency);
            }
            std::cout << "\t[Userspace frequency change test passed]" << std::endl;
        }
    }
}
