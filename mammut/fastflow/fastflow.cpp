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

#include "mammut/fastflow/fastflow.hpp"

namespace mammut{
namespace fastflow{

FarmAdaptivityManager* FarmAdaptivityManager::local(){
    return new FarmAdaptivityManager(NULL);
}

FarmAdaptivityManager* FarmAdaptivityManager::remote(Communicator* const communicator){
    return new FarmAdaptivityManager(communicator);
}

void FarmAdaptivityManager::release(FarmAdaptivityManager* module){
    if(module){
        delete module;
    }
}

FarmAdaptivityManager::FarmAdaptivityManager(Communicator* const communicator):
        _farm(NULL),
        _reconfigurationStrategy(RECONF_STRATEGY_POWER_CONSERVATIVE),
        _frequencyGovernor(cpufreq::GOVERNOR_USERSPACE),
        _numSamples(10),
        _samplingInterval(1),
        _underloadThresholdFarm(80.0),
        _overloadThresholdFarm(90.0),
        _underloadThresholdWorker(80.0),
        _overloadThresholdWorker(90.0),
        _migrateCollector(true),
        _stabilizationPeriod(4){
    if(communicator){
        _cpufreq = cpufreq::CpuFreq::remote(communicator);
        _energy = energy::Energy::remote(communicator);
        _topology = topology::Topology::remote(communicator);
    }else{
        _cpufreq = cpufreq::CpuFreq::local();
        _energy = energy::Energy::local();
        _topology = topology::Topology::local();
    }
}

FarmAdaptivityManager::~FarmAdaptivityManager(){
    cpufreq::CpuFreq::release(_cpufreq);
    energy::Energy::release(_energy);
    topology::Topology::release(_topology);
}

/**
 * Checks if a specific reconfiguration strategy is available.
 * @param strategy The reconfiguration strategy.
 * @return True if the strategy is available, false otherwise.
 */
bool FarmAdaptivityManager::isReconfigurationStrategyAvailable(ReconfigurationStrategy strategy){
    std::vector<cpufreq::Domain*> frequencyDomains = _cpufreq->getDomains().size();
    bool frequenciesAvailable = (frequencyDomains.size() != 0);
    switch(strategy){
        case RECONF_STRATEGY_NO_FREQUENCY:{
            return true;
        }break;
        case RECONF_STRATEGY_OS_FREQUENCY:{
            return frequenciesAvailable;
        }break;
        case RECONF_STRATEGY_CORES_CONSERVATIVE:
        case RECONF_STRATEGY_POWER_CONSERVATIVE:{
            return frequenciesAvailable && isFrequencyGovernorAvailable(cpufreq::GOVERNOR_USERSPACE);
        }break;
    }
}

void FarmAdaptivityManager::initCpuFreq(){
    if(_reconfigurationStrategy != RECONF_STRATEGY_NO_FREQUENCY){
         std::vector<cpufreq::Domain*> frequencyDomains = frequencyDomains = _cpufreq->getDomains();
         if(!frequencyDomains.size()){
             throw std::runtime_error("FarmAdaptivityManager: Impossible to use a reconfiguration "
                                      "strategy with frequency scaling capabilities.");
         }

         if(_reconfigurationStrategy == RECONF_STRATEGY_CORES_CONSERVATIVE ||
            _reconfigurationStrategy == RECONF_STRATEGY_POWER_CONSERVATIVE){
             _frequencyGovernor = cpufreq::GOVERNOR_USERSPACE;
         }

         if(!isFrequencyGovernorAvailable(_frequencyGovernor)){
             throw std::runtime_error("FarmAdaptivityManager: Invalid frequency governor set.");
         }

         std::vector<cpufreq::Frequency> availableFrequencies;
         for(size_t i = 0; i < frequencyDomains.size(); i++){
             availableFrequencies = frequencyDomains.at(i)->getAvailableFrequencies();
             frequencyDomains.at(i)->changeGovernor(_frequencyGovernor);

             if(_frequencyGovernor != cpufreq::GOVERNOR_USERSPACE){
                 //TODO: Add a call to specify bounds in os governor based strategies
                 frequencyDomains.at(i)->changeGovernorBounds(availableFrequencies.at(0),
                                                              availableFrequencies.at(availableFrequencies.size() - 1));
             }else if(_reconfigurationStrategy != RECONF_STRATEGY_OS_FREQUENCY){
                 //TODO: If possible, run emitter and collector at higher frequencies.
                 frequencyDomains.at(i)->changeFrequency(availableFrequencies.at(availableFrequencies.size() - 1));
             }
         }
     }
}

void FarmAdaptivityManager::initMapping(){
    ;
}

void FarmAdaptivityManager::run(){
    if(_farm){
        throw std::runtime_error("FarmAdaptivityManager: setFarm() must be called before run().");
    }

    initCpuFreq();
    initMapping();

    while(true){
        ;
    }
}

void FarmAdaptivityManager::stop(){
}

const ff::ff_farm*& FarmAdaptivityManager::getFarm() const{
    return _farm;
}

void FarmAdaptivityManager::setFarm(const ff::ff_farm*& farm){
    _farm = farm;
}

double FarmAdaptivityManager::getUnderloadThresholdFarm() const{
    return _underloadThresholdFarm;
}

void FarmAdaptivityManager::setUnderloadThresholdFarm(double underloadThresholdFarm){
    _underloadThresholdFarm = underloadThresholdFarm;
}

double FarmAdaptivityManager::getUnderloadThresholdWorker() const{
    return _underloadThresholdWorker;
}

void FarmAdaptivityManager::setUnderloadThresholdWorker(double underloadThresholdWorker){
    _underloadThresholdWorker = underloadThresholdWorker;
}

double FarmAdaptivityManager::getOverloadThresholdFarm() const{
    return _overloadThresholdFarm;
}

void FarmAdaptivityManager::setOverloadThresholdFarm(double overloadThresholdFarm){
    _overloadThresholdFarm = overloadThresholdFarm;
}

double FarmAdaptivityManager::getOverloadThresholdWorker() const{
    return _overloadThresholdWorker;
}

void FarmAdaptivityManager::setOverloadThresholdWorker(double overloadThresholdWorker){
    _overloadThresholdWorker = overloadThresholdWorker;
}

bool FarmAdaptivityManager::isMigrateCollector() const{
    return _migrateCollector;
}

void FarmAdaptivityManager::setMigrateCollector(bool migrateCollector){
    _migrateCollector = migrateCollector;
}

uint32_t FarmAdaptivityManager::getNumSamples() const{
    return _numSamples;
}

void FarmAdaptivityManager::setNumSamples(uint32_t numSamples){
    _numSamples = numSamples;
}

uint32_t FarmAdaptivityManager::getSamplingInterval() const{
    return _samplingInterval;
}

void FarmAdaptivityManager::setSamplingInterval(uint32_t samplingInterval){
    _samplingInterval = samplingInterval;
}

uint32_t FarmAdaptivityManager::getStabilizationPeriod() const{
    return _stabilizationPeriod;
}

bool FarmAdaptivityManager::isFrequencyGovernorAvailable(cpufreq::Governor governor){
    std::vector<cpufreq::Domain*> frequencyDomains = _cpufreq->getDomains();
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

void FarmAdaptivityManager::setStabilizationPeriod(uint32_t stabilizationPeriod){
    _stabilizationPeriod = stabilizationPeriod;
}

ReconfigurationStrategy FarmAdaptivityManager::getReconfigurationStrategy() const{
    return _reconfigurationStrategy;
}

cpufreq::Governor FarmAdaptivityManager::getFrequencyGovernor() const {
    return _frequencyGovernor;
}

void FarmAdaptivityManager::setFrequencyGovernor(cpufreq::Governor frequencyGovernor){
    _frequencyGovernor = frequencyGovernor;
}

void FarmAdaptivityManager::setReconfigurationStrategy(ReconfigurationStrategy reconfigurationStrategy){
    _reconfigurationStrategy = reconfigurationStrategy;
}

}
}
