/*
 * fastflow.cpp
 *
 * Created on: 10/11/2014
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

#include "mammut/utils.hpp"
#include "mammut/fastflow/fastflow.hpp"

namespace mammut{
namespace fastflow{

AdaptivityParameters::AdaptivityParameters(Communicator* const communicator):
    communicator(communicator),
    strategyFrequencies(STRATEGY_FREQUENCY_NO),
    frequencyGovernor(cpufreq::GOVERNOR_USERSPACE),
    strategyMapping(STRATEGY_MAPPING_OS),
    numSamples(10),
    samplingInterval(1),
    underloadThresholdFarm(80.0),
    overloadThresholdFarm(90.0),
    underloadThresholdWorker(80.0),
    overloadThresholdWorker(90.0),
    migrateCollector(true),
    stabilizationPeriod(4){
    if(communicator){
        cpufreq = cpufreq::CpuFreq::remote(this->communicator);
        energy = energy::Energy::remote(this->communicator);
        topology = topology::Topology::remote(this->communicator);
    }else{
        cpufreq = cpufreq::CpuFreq::local();
        energy = energy::Energy::local();
        topology = topology::Topology::local();
    }
}

AdaptivityParameters::~AdaptivityParameters(){
    cpufreq::CpuFreq::release(cpufreq);
    energy::Energy::release(energy);
    topology::Topology::release(topology);
}

bool AdaptivityParameters::isFrequencyGovernorAvailable(cpufreq::Governor governor){
    std::vector<cpufreq::Domain*> frequencyDomains = cpufreq->getDomains();
    if(!frequencyDomains.size()){
        return false;
    }

    for(size_t i = 0; i < frequencyDomains.size(); i++){
        if(!utils::contains<cpufreq::Governor>(frequencyDomains.at(i)->getAvailableGovernors(),
                                               governor)){
            return false;
        }
    }
    return true;
}

AdaptivityParametersValidation AdaptivityParameters::validate(){
    if((underloadThresholdFarm > overloadThresholdFarm) ||
       (underloadThresholdWorker > overloadThresholdWorker) ||
       underloadThresholdFarm < 0 || overloadThresholdFarm > 100 ||
       underloadThresholdWorker < 0 || overloadThresholdWorker > 100){
        return VALIDATION_THRESHOLDS_INVALID;
    }

    if(strategyFrequencies != STRATEGY_FREQUENCY_NO){
        std::vector<cpufreq::Domain*> frequencyDomains = cpufreq->getDomains();
        if(!frequencyDomains.size()){
            return VALIDATION_STRATEGY_FREQUENCY_UNSUPPORTED;
        }

        if(strategyFrequencies != STRATEGY_FREQUENCY_OS){
            frequencyGovernor = cpufreq::GOVERNOR_USERSPACE;
            if(!isFrequencyGovernorAvailable(frequencyGovernor)){
                return VALIDATION_STRATEGY_FREQUENCY_UNSUPPORTED;
            }
        }
    }

    if(!isFrequencyGovernorAvailable(frequencyGovernor)){
        return VALIDATION_GOVERNOR_UNSUPPORTED;
    }

    if(strategyMapping == STRATEGY_MAPPING_CACHE_EFFICIENT){
        return VALIDATION_STRATEGY_MAPPING_UNSUPPORTED;
    }

    return VALIDATION_OK;
}

}
}
