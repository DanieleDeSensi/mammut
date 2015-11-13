/*
 * idleLevels.cpp
 * This is a demo on idle levels structure.
 *
 * Created on: 08/04/2015
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
    Mammut mammut;

    /*******************************************/
    /*              Idle states test           */
    /*******************************************/
    VirtualCore* vc = mammut.getInstanceTopology()->getVirtualCore(12);
    Cpu* cpu = mammut.getInstanceTopology()->getCpu(vc->getCpuId());
    vector<VirtualCoreIdleLevel*> idleLevels = vc->getIdleLevels();
    if(idleLevels.size() == 0){
        cout << "No idle levels supported by CPU " << cpu->getCpuId() << "." << endl;
    }else{
        cout << "The following idle levels are supported by CPU " << cpu->getCpuId() << ":" << endl;
        for(size_t i = 0; i < idleLevels.size(); i++){
            VirtualCoreIdleLevel* level = idleLevels.at(i);
            cout << "[Idle Level: " << level->getLevelId() << "]";
            cout << "[Name: " << level->getName() << "]";
            cout << "[Desc: " << level->getDesc() << "]";
            cout << "[Consumed Power: " << level->getConsumedPower() << "]";
            cout << "[Exit latency: " << level->getExitLatency() << "]";
            cout << "[Absolute Time: " << level->getAbsoluteTime() << "]";
            cout << "[Absolute Count: " << level->getAbsoluteCount() << "]";
            cout << "[Enableable: " << level->isEnableable() << "]";
            cout << "[Enabled: " << level->isEnabled() << "]";
            cout << endl;
        }
    }
}
