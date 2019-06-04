#include <mammut/mammut.hpp>

#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

const double levelTime = 10;

using namespace mammut;
using namespace mammut::cpufreq;
using namespace mammut::energy;
using namespace mammut::topology;
using namespace std;

int main(int argc, char** argv){
    if(argc < 2){
        cerr << "Usage: " << argv[0] << " CpuId" << endl;
    }

    unsigned int cpuId = atoi(argv[1]);

    Mammut mammut;

    /*******************************************/
    /*       Idle states power consumption     */
    /*******************************************/
    VirtualCore* vc = mammut.getInstanceTopology()->getCpus().at(cpuId)->getVirtualCore();
    Cpu* cpu = mammut.getInstanceTopology()->getCpu(vc->getCpuId());
    vector<VirtualCoreIdleLevel*> idleLevels = vc->getIdleLevels();
    Domain* fDomain = mammut.getInstanceCpuFreq()->getDomain(vc);
    if(idleLevels.size() == 0){
        cout << "No idle levels supported by CPU " << cpu->getCpuId() << "." << endl;
    }else{
        for(int32_t i = idleLevels.size() - 1; i >= 0 ; i--){
            int fd = open("/dev/cpu_dma_latency", O_RDWR);
            int32_t lat = idleLevels.at(i)->getExitLatency();
            if(write(fd, &lat, sizeof(lat)) != sizeof(lat)){
                throw std::runtime_error("Error while writing\n");
            }

            /** We compute the base power consumption at each frequency step. **/
            vector<Frequency> frequencies;
            frequencies = fDomain->getAvailableFrequencies();

            for(size_t j = 0; j < frequencies.size(); j++){
                fDomain->setGovernor(GOVERNOR_USERSPACE);
                fDomain->setFrequencyUserspace(frequencies.at(j));
                CounterCpus* counter = dynamic_cast<CounterCpus*>(mammut.getInstanceEnergy()->getCounter(COUNTER_CPUS));
                counter->reset();
                sleep(levelTime);

                mammut::topology::CpuId cpuId = cpu->getCpuId();
                cout << idleLevels.at(i)->getName() << " ";
                cout << frequencies.at(j) << " ";
                cout << counter->getJoulesCpu(cpuId)/levelTime << " ";
                cout << counter->getJoulesCores(cpuId)/levelTime << " ";
                cout << counter->getJoulesDram(cpuId)/levelTime << " ";
                cout << counter->getJoulesGraphic(cpuId)/levelTime << " ";
                cout << fDomain->getCurrentVoltage() << " ";
                cout << endl;
            }

            close(fd);
        }
    }
}
