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

AdaptiveNode::AdaptiveNode():
    _tasksManager(NULL),
    _thread(NULL),
    _tasksCount(0),
    _workTicks(0),
    _startTicks(getticks()){
    ff::init_unlocked(_lock);
}

AdaptiveNode::~AdaptiveNode(){
    if(_thread){
        _tasksManager->releaseThreadHandler(_thread);
    }

    if(_tasksManager){
        task::TasksManager::release(_tasksManager);
    }
}

void AdaptiveNode::waitThreadCreation(){
    _threadCreated.wait();
}

task::ThreadHandler* AdaptiveNode::getThreadHandler() const{
    if(_thread){
        return _thread;
    }else{
        throw std::runtime_error("AdaptiveNode: Thread not initialized.");
    }
}

void AdaptiveNode::initMammutModules(Communicator* const communicator){
    if(communicator){
        _tasksManager = task::TasksManager::remote(communicator);
    }else{
        _tasksManager = task::TasksManager::local();
    }
}

void AdaptiveNode::getAndResetStatistics(double& workPercentage, uint64_t& tasksCount){
    ff::spin_lock(_lock);
    ticks now = getticks();
    workPercentage = ((double) _workTicks / (double)(now - _startTicks)) * 100.0;
    tasksCount = _tasksCount;
    _workTicks = 0;
    _startTicks = now;
    _tasksCount = 0;
    ff::spin_unlock(_lock);
}

int AdaptiveNode::adp_svc_init(){return 0;}

int AdaptiveNode::svc_init() CX11_KEYWORD(final){
    if(!_threadCreated.predicate()){
        /** Operations performed only the first time the thread is running. **/
        if(_tasksManager){
            _thread = _tasksManager->getThreadHandler();
        }else{
            throw std::runtime_error("AdaptiveWorker: Tasks manager not initialized.");
        }
        _threadCreated.notifyOne();
    }
    std::cout << "My svc_init called." << std::endl;
    return adp_svc_init();
}

void* AdaptiveNode::svc(void* task) CX11_KEYWORD(final){
    std::cout << "My svc called." << std::endl;
    ticks start = getticks();
    void* t = adp_svc(task);
    ff::spin_lock(_lock);
    ++_tasksCount;
    _workTicks += getticks() - start;
    ff::spin_unlock(_lock);
    return t;
}

AdaptivityParameters::AdaptivityParameters(Communicator* const communicator):
    communicator(communicator),
    strategyMapping(STRATEGY_MAPPING_NO),
    strategyFrequencies(STRATEGY_FREQUENCY_NO),
    frequencyGovernor(cpufreq::GOVERNOR_USERSPACE),
    strategyNeverUsedVirtualCores(STRATEGY_UNUSED_VC_NONE),
    strategyUnusedVirtualCores(STRATEGY_UNUSED_VC_NONE),
    sensitiveEmitter(false),
    sensitiveCollector(false),
    numSamples(10),
    samplingInterval(1),
    underloadThresholdFarm(80.0),
    overloadThresholdFarm(90.0),
    underloadThresholdWorker(80.0),
    overloadThresholdWorker(90.0),
    migrateCollector(true),
    stabilizationPeriod(4),
    frequencyLowerBound(0),
    frequencyUpperBound(0){
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

AdaptivityParametersValidation AdaptivityParameters::validate(){
    std::vector<cpufreq::Domain*> frequencyDomains = cpufreq->getDomains();
    std::vector<topology::VirtualCore*> virtualCores = topology->getVirtualCores();

    if(strategyFrequencies != STRATEGY_FREQUENCY_NO && strategyMapping == STRATEGY_MAPPING_NO){
        return VALIDATION_STRATEGY_FREQUENCY_REQUIRES_MAPPING;
    }

    /** Validate thresholds. **/
    if((underloadThresholdFarm > overloadThresholdFarm) ||
       (underloadThresholdWorker > overloadThresholdWorker) ||
       underloadThresholdFarm < 0 || overloadThresholdFarm > 100 ||
       underloadThresholdWorker < 0 || overloadThresholdWorker > 100){
        return VALIDATION_THRESHOLDS_INVALID;
    }

    /** Validate frequency strategies. **/
    if(strategyFrequencies != STRATEGY_FREQUENCY_NO){
        if(!frequencyDomains.size()){
            return VALIDATION_STRATEGY_FREQUENCY_UNSUPPORTED;
        }

        if(strategyFrequencies != STRATEGY_FREQUENCY_OS){
            frequencyGovernor = cpufreq::GOVERNOR_USERSPACE;
            if(!cpufreq->isGovernorAvailable(frequencyGovernor)){
                return VALIDATION_STRATEGY_FREQUENCY_UNSUPPORTED;
            }
        }
        if((sensitiveEmitter || sensitiveCollector) &&
           !cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_PERFORMANCE) &&
           !cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_USERSPACE)){
            return VALIDATION_EC_SENSITIVE_MISSING_GOVERNORS;
        }

    }else{
        if(sensitiveEmitter || sensitiveCollector){
            return VALIDATION_EC_SENSITIVE_WRONG_F_STRATEGY;
        }
    }

    /** Validate governor availability. **/
    if(!cpufreq->isGovernorAvailable(frequencyGovernor)){
        return VALIDATION_GOVERNOR_UNSUPPORTED;
    }

    /** Validate mapping strategy. **/
    if(strategyMapping == STRATEGY_MAPPING_CACHE_EFFICIENT){
        return VALIDATION_STRATEGY_MAPPING_UNSUPPORTED;
    }

    /** Validate frequency bounds. **/
    if(frequencyLowerBound || frequencyUpperBound){
        if(strategyFrequencies == STRATEGY_FREQUENCY_OS){
            std::vector<cpufreq::Frequency> availableFrequencies = frequencyDomains.at(0)->getAvailableFrequencies();
            if(!availableFrequencies.size()){
                return VALIDATION_INVALID_FREQUENCY_BOUNDS;
            }

            if(frequencyLowerBound){
                if(!utils::contains(availableFrequencies, frequencyLowerBound)){
                    return VALIDATION_INVALID_FREQUENCY_BOUNDS;
                }
            }else{
                frequencyLowerBound = availableFrequencies.at(0);
            }

            if(frequencyUpperBound){
                if(!utils::contains(availableFrequencies, frequencyUpperBound)){
                    return VALIDATION_INVALID_FREQUENCY_BOUNDS;
                }
            }else{
                frequencyUpperBound = availableFrequencies.back();
            }
        }else{
            return VALIDATION_INVALID_FREQUENCY_BOUNDS;
        }
    }

    /** Validate unused cores strategy. **/
    switch(strategyUnusedVirtualCores){
        case STRATEGY_UNUSED_VC_OFF:{
            bool hotPluggableFound = false;
            for(size_t i = 0; i < virtualCores.size(); i++){
                if(virtualCores.at(i)->isHotPluggable()){
                    hotPluggableFound = true;
                }
            }
            if(!hotPluggableFound){
                return VALIDATION_UNUSED_VC_NO_OFF;
            }
        }break;
        case STRATEGY_UNUSED_VC_LOWEST_FREQUENCY:{
            if(!cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_POWERSAVE) &&
               !cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_USERSPACE)){
                return VALIDATION_UNUSED_VC_NO_FREQUENCIES;
            }
        }break;
        default:
            break;
    }

    return VALIDATION_OK;
}

}
}
