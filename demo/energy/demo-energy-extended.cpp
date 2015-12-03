/*
 * demo-energy-extended.cpp
 *
 * Created on: 04/12/2014
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
    CounterPlug* counterPlug = energy->getCounterPlug();
    if(!counterPlug){
        cout << "Plug counter not present on this machine." << endl;
    }

    /** Gets the CPUs energy counters. **/
    CounterCpus* counterCpus = energy->getCounterCpus();
    if(!counterCpus){
        cout << "Cpu counters not present on this machine." << endl;
    }

    /** Prints information for each counter. **/
    if(counterCpus){
        cout << "Found Cpus counter ";
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
            cout << "Joules consumed at power plug: " << counterPlug->getValue() << endl;
        }

        vector<Cpu*> cpus = m.getInstanceTopology()->getCpus();
        for(size_t j = 0; j < cpus.size(); j++){
            CpuId id = cpus.at(j)->getCpuId();
            cout << "Joules consumed for CPU " << id << ": ";
            cout << "Cpu: " << counterCpus->getJoulesCpu(id) << " ";
            cout << "Cores: " << counterCpus->getJoulesCores(id) << " ";
            if(counterCpus->hasJoulesGraphic()){cout << "Graphic: " << counterCpus->getJoulesGraphic(id) << " ";}
            if(counterCpus->hasJoulesDram()){cout << "Dram: " << counterCpus->getJoulesDram(id) << " ";}
            cout << endl;
        }
        counterCpus->reset();
    }
}
