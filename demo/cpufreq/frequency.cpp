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
