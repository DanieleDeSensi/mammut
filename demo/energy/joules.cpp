/*
 * joules.cpp
 * This is a minimal demo on local energy counters reading.
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
using namespace std;

int main(int argc, char** argv){
    Mammut m;
    Energy* energy = m.getInstanceEnergy();

    /** Gets the energy counters (one per CPU). **/
    CounterCpus* counterCpus = energy->getCounterCpus();
    if(!counterCpus){
        cout << "Cpu counters not present on this machine." << endl;
        return -1;
    }

    unsigned int sleepingSecs = 10;
    cout << "Sleeping " << sleepingSecs << " seconds." << endl;
    sleep(sleepingSecs);

    cout << "Value: " << counterCpus->getValue() << " ";
    cout << "Total Cpus Joules: " << counterCpus->getJoulesCpuAll() << " ";
    cout << "Total Cores Joules: " << counterCpus->getJoulesCoresAll() << " ";
    cout << "Total Dram Joules: " << counterCpus->getJoulesDramAll() << " ";
    cout << "Total Graphic Joules: " << counterCpus->getJoulesGraphicAll() << " ";
    cout << endl;
}
