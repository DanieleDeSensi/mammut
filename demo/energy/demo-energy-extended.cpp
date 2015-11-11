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

#include <iostream>
#include <unistd.h>

using namespace mammut;
using namespace mammut::energy;
using namespace std;

int main(int argc, char** argv){
    Mammut m;
    Energy* energy = m.getInstanceEnergy();

    /** Gets the energy counters (one per CPU). **/
    vector<CounterCpu*> counters = energy->getCountersCpu();
    if(counters.size() == 0){
        cout << "Cpu counters not present on this machine." << endl;
        return 0;
    }

    /** Prints information for each counter. **/
    for(size_t i = 0; i < counters.size(); i++){
        CounterCpu* c = counters.at(i);
        cout << "Found Cpu counter for cpu: " << c->getCpu()->getCpuId() << " ";
        cout << "Has graphic counter: " << c->hasJoulesGraphic() << " ";
        cout << "Has Dram counter: " << c->hasJoulesDram() << " ";
        cout << endl;
    }

    /** Gets the value of the counter every sleepingSecs seconds. **/
    unsigned int sleepingSecs = 10;
    unsigned int iterations = 4;
    for(unsigned int i = 0; i < iterations; i++){
        cout << "Sleeping " << sleepingSecs << " seconds." << endl;
        sleep(sleepingSecs);
        for(size_t j = 0; j < counters.size(); j++){
            CounterCpu* c = counters.at(j);
            cout << "Joules consumed for CPU " << c->getCpu()->getCpuId() << " in the last " << sleepingSecs << " seconds: ";
            cout << "Cpu: " << c->getJoulesCpu() << " ";
            cout << "Cores: " << c->getJoulesCores() << " ";
            if(c->hasJoulesGraphic()){cout << "Graphic: " << c->getJoulesGraphic() << " ";}
            if(c->hasJoulesDram()){cout << "Dram: " << c->getJoulesDram() << " ";}
            cout << endl;
            c->reset();
        }
    }
}
