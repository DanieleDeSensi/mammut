/*
 * idleLevelsPower.cpp
 * This is a demo on idle levels power consumption.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

const double levelTime = 10;

using namespace mammut;
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
    vector<VirtualCore*> virtualCores = cpu->getVirtualCores();
    vector<VirtualCoreIdleLevel*> idleLevels = vc->getIdleLevels();
    mammut::cpufreq::Domain* fDomain = mammut.getInstanceCpuFreq()->getDomain(vc);
    if(idleLevels.size() == 0){
        cout << "No idle levels supported by CPU " << cpu->getCpuId() << "." << endl;
    }else{
        for(int32_t i = idleLevels.size() - 1; i >= 0 ; i--){
            int fd = open("/dev/cpu_dma_latency", O_RDWR);
            int32_t lat = idleLevels.at(i)->getExitLatency();
            write(fd, &lat, sizeof(lat));

            /** We compute the base power consumption at each frequency step. **/
            vector<mammut::cpufreq::Frequency> frequencies;
            frequencies = fDomain->getAvailableFrequencies();

            for(size_t j = 0; j < frequencies.size(); j++){
                fDomain->setGovernor(mammut::cpufreq::GOVERNOR_USERSPACE);
                fDomain->setFrequencyUserspace(frequencies.at(j));
                mammut::energy::CounterCpus* counter = mammut.getInstanceEnergy()->getCounterCpus();
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
