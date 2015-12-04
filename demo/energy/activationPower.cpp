/*
 * activationPower.cpp
 * Computes the power needed to activate one voltage domain.
 *
 * Created on: 07/05/2015
 *
 * =========================================================================
 *  Copyright (C) 2015-, Daniele De Sensi (d.desensi.software@gmail.com)
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
    CounterCpus* counter = (CounterCpus*) energy->getCounter(COUNTER_CPUS);
    vector<PhysicalCore*> physicalCores = mammut.getInstanceTopology()->virtualToPhysical(domain->getVirtualCores());

    RollbackPoint rp = domain->getRollbackPoint();
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

    domain->rollback(rp);
}
