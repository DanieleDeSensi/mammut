/*
 * offlining.cpp
 * This is a minimal demo on cores offlining.
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
    vector<VirtualCore*> cores = topology->getVirtualCores();

    // Pick a virtual core
    VirtualCore* core = cores.back();
    if(core->isHotPluggable()){
        core->hotUnplug();
        if(core->isHotPlugged()){
            std::cout << "Failed to offline the core." << std::endl;
        }else{
            std::cout << "Core offlined." << std::endl;
        }
        core->hotPlug();
	std::cout << "Core online again." << std::endl;
    }else{
        std::cout << "Core cannot be offlined." << std::endl;
    }
}
