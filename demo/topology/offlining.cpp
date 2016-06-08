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
