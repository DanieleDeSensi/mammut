/*
 * demo-topology.cpp
 * This is a minimal demo on topology reading.
 *
 * Created on: 07/01/2015
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

using namespace mammut;
using namespace mammut::topology;
using namespace std;

int main(int argc, char** argv){
    Mammut m;
    Topology* topology = m.getInstanceTopology();

    /*******************************************/
    /*               Topology test             */
    /*******************************************/
    vector<Cpu*> cpus = topology->getCpus();
    cout << "The machine has [" << cpus.size() << " CPUs]" << endl;
    for(size_t i = 0; i < cpus.size(); i++){
        Cpu* cpu = cpus.at(i);
        cout << "[CPU Id: " << cpu->getCpuId() << "]";
        cout << "[Vendor: " << cpu->getVendorId() << "]";
        cout << "[Family: " << cpu->getFamily() << "]";
        cout << "[Model: " << cpu->getModel() << "]";
        uint numPhysicalCores = cpu->getPhysicalCores().size();
        uint numVirtualCores = cpu->getVirtualCores().size();
        cout << "[" << numPhysicalCores << " physical cores ";
        if(numPhysicalCores == numVirtualCores){
            cout << "without hyperthreading ";
        }else{
            cout << "with " << numVirtualCores / numPhysicalCores << " ways hyperthreading ";
        }
        cout << "(" << numVirtualCores << " virtual cores)]";

        cout << endl;
    }


    vector<VirtualCore*> virtualCores = topology->getVirtualCores();
    VirtualCore* pluggable = NULL;

    for(size_t i = 0; i < virtualCores.size(); i++){
        VirtualCore* vc = virtualCores.at(i);
        cout << "[CpuId " << vc->getCpuId() << "]";
        cout << "[PhysicalCoreId: " << vc->getPhysicalCoreId() << "]";
        cout << "[VirtualCore Id: " << vc->getVirtualCoreId() << "]";
        cout << "[Hotplug: ";
        if(!vc->isHotPluggable()){
            cout << "not supported]";
        }else{
            pluggable = vc;
            if(vc->isHotPlugged()){
                cout << "plugged]";
            }else{
                cout << "unplugged]";
            }
        }
        cout << "[Constant TSC: " << vc->areTicksConstant() << "]";
        cout << endl;
    }

    /*******************************************/
    /*              HotPlug test               */
    /*******************************************/

    if(pluggable){
        cout << "Virtual " << pluggable->getVirtualCoreId() << " is hot pluggable. "
                     "Plugged: " << pluggable->isHotPlugged() << endl;;
        cout << "Unplugging.." << endl;
        pluggable->hotUnplug();
        assert(!pluggable->isHotPlugged());
        cout << "Plugging.." << endl;
        pluggable->hotPlug();
        assert(pluggable->isHotPlugged());
        cout << "Plugging test successful" << endl;
    }

    /*******************************************/
    /*              Utilisation test           */
    /*******************************************/
    for(size_t i = 0; i < virtualCores.size(); i++){
        virtualCores.at(i)->resetIdleTime();
    }

    vector<VirtualCoreIdleLevel*> idleLevels = virtualCores.at(0)->getIdleLevels();
    if(idleLevels.size()){
        for(size_t i = 0; i < idleLevels.size(); i++){
            idleLevels.at(i)->resetTime();
            idleLevels.at(i)->resetCount();
        }
    }

    uint sleepingSecs = 10;
    cout << "Maximizing utilization of virtual core 0 for " << sleepingSecs << " seconds." << endl;
    virtualCores.at(0)->maximizeUtilization();
    sleep(sleepingSecs);
    virtualCores.at(0)->resetUtilization();

    for(size_t i = 0; i < virtualCores.size(); i++){
        VirtualCore* tmp = virtualCores.at(i);
        cout << "[Virtual Core " << tmp->getVirtualCoreId() << "] " << ((((double)tmp->getIdleTime())/1000000.0) / ((double)sleepingSecs)) * 100.0 << "% idle" << endl;
    }

    if(idleLevels.size()){
        uint totalTime = 0;
        for(size_t i = 0; i < idleLevels.size(); i++){
            VirtualCoreIdleLevel* level = idleLevels.at(i);
            uint time = level->getTime();
            cout << "[Level: " << level->getName() << "][Time: " << time << "][Count: " << level->getCount() << "]" << endl;
            totalTime += time;
        }
        cout << "Total idle time according to /proc/stat: " << (double)virtualCores.at(idleLevels.at(0)->getVirtualCoreId())->getIdleTime() / 1000000.0 << endl;
        cout << "Total idle time according to idle states: " << totalTime / 1000000.0 << endl;
        cout << "Total C0 time according to idle states: " << sleepingSecs - (totalTime / 1000000.0) << endl;;
    }
}
