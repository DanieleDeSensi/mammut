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
    VirtualCore* vc = mammut.getInstanceTopology()->getVirtualCore(0);
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
