/*
 * demo-frequency.cpp
 *
 * Created on: 15/01/2015
 *
 * =========================================================================
 *  Copyright (C) 2014-, Daniele De Sensi (d.desensi.software@gmail.com)
 *
 *  This file is part of mammut.
 *
 *  mammut is free software: you can redistribute it and/or
 *  modify it under the terms of the Lesser GNU General Public
 *  License as published by the Free Software Foundation, either
 *  version 3 of the License, or (at your option) any later version.

 *  mammut is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  Lesser GNU General Public License for more details.
 *
 *  You should have received a copy of the Lesser GNU General Public
 *  License along with mammut.
 *  If not, see <http://www.gnu.org/licenses/>.
 *
 * =========================================================================
 */

#include <mammut/communicator-tcp.hpp>
#include <mammut/cpufreq/cpufreq.hpp>

#include <iostream>
#include <unistd.h>

int main(int argc, char** argv){
    mammut::CommunicatorTcp* communicator = NULL;
    std::cout << "Usage: " << argv[0] << " [TcpAddress:TcpPort]" << std::endl;

    /** Gets the address and the port of the server and builds the communicator. **/
    if(argc > 1){
        std::string addressPort = argv[1];
        size_t pos = addressPort.find_first_of(":");
        std::string address = addressPort.substr(0, pos);
        uint16_t port = atoi(addressPort.substr(pos + 1).c_str());
        communicator = new mammut::CommunicatorTcp(address, port);
    }

    mammut::cpufreq::CpuFreq* frequency;

    /** A local or a remote handler is built. **/
    if(communicator){
        frequency = mammut::cpufreq::CpuFreq::remote(communicator);
    }else{
        frequency = mammut::cpufreq::CpuFreq::local();
    }

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
    std::vector<mammut::cpufreq::Domain*> domains = frequency->getDomains();
    std::cout << "[" << domains.size() << " frequency domains found]" << std::endl;
    for(size_t i = 0; i < domains.size(); i++){
        bool userspaceAvailable = false;
        mammut::cpufreq::Domain* domain = domains.at(i);
        std::vector<mammut::topology::VirtualCoreId> identifiers = domain->getVirtualCoresIdentifiers();

        std::cout << "[Domain " << domain->getId() << "]";
        std::cout << "[Virtual Cores: ";
        for(size_t j = 0; j < identifiers.size(); j++){
            std::cout << identifiers.at(j) << ", ";
        }
        std::cout << "]" << std::endl;

        std::cout << "\tAvailable Governors: [";
        std::vector<mammut::cpufreq::Governor> governors = domain->getAvailableGovernors();
        for(size_t j = 0; j < governors.size() ; j++){
            if(governors.at(j) == mammut::cpufreq::GOVERNOR_USERSPACE){
                userspaceAvailable = true;
            }
            std::cout << frequency->getGovernorNameFromGovernor(governors.at(j)) << ", ";
        }
        std::cout << "]" << std::endl;
        std::cout << "\tAvailable Frequencies: [";
        std::vector<mammut::cpufreq::Frequency> frequencies = domain->getAvailableFrequencies();
        for(size_t j = 0; j < frequencies.size() ; j++){
            std::cout << frequencies.at(j) << "KHz, ";
        }
        std::cout << "]" << std::endl;

        mammut::cpufreq::Governor currentGovernor = domain->getCurrentGovernor();
        std::cout << "\tCurrent Governor: [" << frequency->getGovernorNameFromGovernor(currentGovernor) << "]" << std::endl;
        mammut::cpufreq::Frequency lb, ub, currentFrequency;
        domain->getHardwareFrequencyBounds(lb, ub);
        std::cout << "\tHardware Frequency Bounds: [" << lb << "KHz, " << ub << "KHz]" << std::endl;
        if(currentGovernor == mammut::cpufreq::GOVERNOR_USERSPACE){
            currentFrequency = domain->getCurrentFrequencyUserspace();
        }else{
            currentFrequency = domain->getCurrentFrequency();
            assert(domain->getCurrentGovernorBounds(lb, ub));
            std::cout << "\tCurrent Governor Bounds: [" << lb << "KHz, " << ub << "KHz]" << std::endl;
        }
        std::cout << "\tCurrent Frequency: [" << currentFrequency << "]" << std::endl;

        /** Change frequency test. **/
        if(userspaceAvailable && frequencies.size()){
            domain->changeGovernor(mammut::cpufreq::GOVERNOR_USERSPACE);
            assert(domain->getCurrentGovernor() == mammut::cpufreq::GOVERNOR_USERSPACE);
            domain->changeFrequency(frequencies.at(0));
            assert(domain->getCurrentFrequencyUserspace() == frequencies.at(0));
            domain->changeFrequency(frequencies.at(frequencies.size() - 1));
            assert(domain->getCurrentFrequencyUserspace() == frequencies.at(frequencies.size() - 1));
            /** Restore original governor and frequency. **/
            domain->changeGovernor(currentGovernor);
            assert(currentGovernor == currentGovernor);
            if(currentGovernor == mammut::cpufreq::GOVERNOR_USERSPACE){
                domain->changeFrequency(currentFrequency);
                assert(domain->getCurrentFrequencyUserspace() == currentFrequency);
            }
            std::cout << "\t[Userspace frequency change test passed]" << std::endl;
        }
    }

    mammut::cpufreq::CpuFreq::release(frequency);
}
