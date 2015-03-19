#ifndef MAMMUT_FASTFLOW_TPP_
#define MAMMUT_FASTFLOW_TPP_

#include <cmath>
#include <limits>

namespace mammut{
namespace fastflow{

template <typename lb_t, typename gt_t>
std::vector<AdaptiveNode*> AdaptiveFarm<lb_t, gt_t>::getAdaptiveWorkers() const{
    return _adaptiveWorkers;
}

template <typename lb_t, typename gt_t>
AdaptiveNode* AdaptiveFarm<lb_t, gt_t>::getAdaptiveEmitter() const{
    return _adaptiveEmitter;
}

template <typename lb_t, typename gt_t>
AdaptiveNode* AdaptiveFarm<lb_t, gt_t>::getAdaptiveCollector() const{
    return _adaptiveCollector;
}

template <typename lb_t, typename gt_t>
void AdaptiveFarm<lb_t, gt_t>::construct(AdaptivityParameters* adaptivityParameters){
    _adaptiveEmitter = NULL;
    _adaptiveCollector = NULL;
    _firstRun = true;
    _adaptivityParameters = adaptivityParameters;
    _adaptivityManager = NULL;
    uint validationRes = _adaptivityParameters->validate();
    if(validationRes != VALIDATION_OK){
        throw std::runtime_error("AdaptiveFarm: invalid AdaptivityParameters: " + utils::intToString(validationRes));
    }
}

template <typename lb_t, typename gt_t>
AdaptiveFarm<lb_t, gt_t>::AdaptiveFarm(AdaptivityParameters* adaptivityParameters, std::vector<ff_node*>& w,
                                       ff_node* const emitter, ff_node* const collector, bool inputCh):
    ff_farm<lb_t, gt_t>::ff_farm(w, emitter, collector, inputCh){
    construct(adaptivityParameters);
}

template <typename lb_t, typename gt_t>
AdaptiveFarm<lb_t, gt_t>::AdaptiveFarm(AdaptivityParameters* adaptivityParameters, bool inputCh,
                                       int inBufferEntries,
                                       int outBufferEntries,
                                       bool workerCleanup,
                                       int maxNumWorkers,
                                       bool fixedSize):
    ff_farm<lb_t, gt_t>::ff_farm(inputCh, inBufferEntries, outBufferEntries, workerCleanup, maxNumWorkers, fixedSize){
    construct(adaptivityParameters);
}

template <typename lb_t, typename gt_t>
AdaptiveFarm<lb_t, gt_t>::~AdaptiveFarm(){
    ;
}

template <typename lb_t, typename gt_t>
int AdaptiveFarm<lb_t, gt_t>::run(bool skip_init){
    if(_firstRun){
        svector<ff_node*> workers = ff_farm<lb_t, gt_t>::getWorkers();
        for(size_t i = 0; i < workers.size(); i++){
            _adaptiveWorkers.push_back(static_cast<AdaptiveNode*>(workers[i]));
            _adaptiveWorkers.at(i)->initMammutModules(_adaptivityParameters->communicator);
        }

        _adaptiveEmitter = static_cast<AdaptiveNode*>(ff_farm<lb_t, gt_t>::getEmitter());
        if(_adaptiveEmitter){
            _adaptiveEmitter->initMammutModules(_adaptivityParameters->communicator);
        }

        _adaptiveCollector = static_cast<AdaptiveNode*>(ff_farm<lb_t, gt_t>::getCollector());
        if(_adaptiveCollector){
            _adaptiveCollector->initMammutModules(_adaptivityParameters->communicator);
        }
    }

    int r = ff_farm<lb_t, gt_t>::run(skip_init);
    if(r){
        return r;
    }

    if(_firstRun){
        _firstRun = false;
        _adaptivityManager = new AdaptivityManagerFarm<lb_t, gt_t>(this, _adaptivityParameters);
        _adaptivityManager->start();
    }
    return r;
}

template <typename lb_t, typename gt_t>
int AdaptiveFarm<lb_t, gt_t>::wait(){
    if(_adaptivityManager){
        _adaptivityManager->join();
        delete _adaptivityManager;
    }
    return ff_farm<lb_t, gt_t>::wait();
}

template<typename lb_t, typename gt_t>
AdaptivityManagerFarm<lb_t, gt_t>::AdaptivityManagerFarm(AdaptiveFarm<lb_t, gt_t>* farm, AdaptivityParameters* adaptivityParameters):
    _stop(false),
    _farm(farm),
    _p(adaptivityParameters),
    _emitter(_farm->getAdaptiveEmitter()),
    _collector(_farm->getAdaptiveCollector()),
    _activeWorkers(_farm->getAdaptiveWorkers()),
    _maxNumWorkers(_activeWorkers.size()),
    _emitterSensitivitySatisfied(false),
    _collectorSensitivitySatisfied(false),
    _currentConfiguration(_activeWorkers.size(), 0),
    _availableVirtualCores(getAvailableVirtualCores()),
    _emitterVirtualCore(NULL),
    _collectorVirtualCore(NULL),
    _elapsedSamples(0){
    /** If voltage table file is specified, then load the table. **/
    if(_p->voltageTableFile.compare("")){
        cpufreq::loadVoltageTable(_voltageTable, _p->voltageTableFile);
    }
}

template<typename lb_t, typename gt_t>
AdaptivityManagerFarm<lb_t, gt_t>::~AdaptivityManagerFarm(){
    ;
}

template<typename lb_t, typename gt_t>
std::vector<topology::PhysicalCore*> AdaptivityManagerFarm<lb_t, gt_t>::getSeparatedDomainPhysicalCores(const std::vector<topology::VirtualCore*>& virtualCores) const{
    std::vector<cpufreq::Domain*> allDomains = _p->cpufreq->getDomains();
    std::vector<cpufreq::Domain*> hypotheticWorkersDomains = _p->cpufreq->getDomains(virtualCores);
    std::vector<topology::PhysicalCore*> physicalCoresInUnusedDomains;
    if(allDomains.size() > hypotheticWorkersDomains.size()){
       for(size_t i = 0; i < allDomains.size(); i++){
           cpufreq::Domain* currentDomain = allDomains.at(i);
           if(!utils::contains(hypotheticWorkersDomains, currentDomain)){
               utils::insertToEnd(_p->topology->virtualToPhysical(currentDomain->getVirtualCores()),
                                  physicalCoresInUnusedDomains);
           }
       }
    }
    return physicalCoresInUnusedDomains;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::setDomainToHighestFrequency(const cpufreq::Domain* domain){
    if(!domain->setGovernor(mammut::cpufreq::GOVERNOR_PERFORMANCE)){
        if(!domain->setGovernor(mammut::cpufreq::GOVERNOR_USERSPACE) ||
           !domain->setHighestFrequencyUserspace()){
            throw std::runtime_error("AdaptivityManagerFarm: Fatal error while setting highest frequency for "
                                     "sensitive emitter/collector. Try to run it without sensitivity parameters.");
        }
    }
}

template<typename lb_t, typename gt_t>
inline std::vector<topology::VirtualCore*> AdaptivityManagerFarm<lb_t, gt_t>::getAvailableVirtualCores(){
    std::vector<topology::VirtualCore*> r;
    if(_p->strategyMapping == STRATEGY_MAPPING_AUTO){
        _p->strategyMapping = STRATEGY_MAPPING_LINEAR;
    }

    switch(_p->strategyMapping){
        case STRATEGY_MAPPING_LINEAR:{
           /*
            * Generates a vector of virtual cores to be used for linear mapping.node
            * It contains first one virtual core per physical core (virtual cores
            * on the same CPU are consecutive).
            * Then, the other groups of virtual cores follow.
            */
            std::vector<topology::Cpu*> cpus = _p->topology->getCpus();

            size_t virtualUsed = 0;
            size_t virtualPerPhysical = _p->topology->getVirtualCores().size() /
                                        _p->topology->getPhysicalCores().size();
            while(virtualUsed < virtualPerPhysical){
                for(size_t i = 0; i < cpus.size(); i++){
                    std::vector<topology::PhysicalCore*> physicalCores = cpus.at(i)->getPhysicalCores();
                    for(size_t j = 0; j < physicalCores.size(); j++){
                        r.push_back(physicalCores.at(j)->getVirtualCores().at(virtualUsed));
                    }
                }
                ++virtualUsed;
            }
        }break;
        case STRATEGY_MAPPING_CACHE_EFFICIENT:{
            throw std::runtime_error("Not yet supported.");
        }
        default:
            break;
    }
    return r;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::manageSensitiveNodes(){
    if(_p->sensitiveEmitter && !_emitter){
        _p->sensitiveEmitter = false;
    }

    if(_p->sensitiveCollector && !_collector){
        _p->sensitiveCollector = false;
    }

    if(_p->strategyFrequencies != STRATEGY_FREQUENCY_NO &&
      ((_p->sensitiveEmitter && !_emitterSensitivitySatisfied) ||
       (_p->sensitiveCollector && !_collectorSensitivitySatisfied))){
        size_t scalableVirtualCoresNum = _activeWorkers.size() +
                                         (_emitter && !_p->sensitiveEmitter) +
                                         (_collector && !_p->sensitiveCollector);
        /** When sensitive is specified, we always choose the WEC mapping. **/
        std::vector<topology::VirtualCore*> scalableVirtualCores(_availableVirtualCores.begin(), (scalableVirtualCoresNum < _availableVirtualCores.size())?
                                                                                               _availableVirtualCores.begin() + scalableVirtualCoresNum:
                                                                                               _availableVirtualCores.end());
        std::vector<topology::PhysicalCore*> performancePhysicalCores;
        performancePhysicalCores = getSeparatedDomainPhysicalCores(scalableVirtualCores);
        if(performancePhysicalCores.size()){
            size_t index = 0;

            if(_p->sensitiveEmitter){
                topology::VirtualCore* vc = performancePhysicalCores.at(index)->getVirtualCore();
                setDomainToHighestFrequency(_p->cpufreq->getDomain(vc));
                _emitterVirtualCore = vc;
                _emitterSensitivitySatisfied = true;
                index = (index + 1) % performancePhysicalCores.size();
            }

            if(_p->sensitiveCollector){
                topology::VirtualCore* vc = performancePhysicalCores.at(index)->getVirtualCore();
                setDomainToHighestFrequency(_p->cpufreq->getDomain(vc));
                _collectorVirtualCore = vc;
                _collectorSensitivitySatisfied = true;
                index = (index + 1) % performancePhysicalCores.size();
            }
        }
    }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::getMappingIndexes(size_t& emitterIndex, size_t& firstWorkerIndex, size_t& collectorIndex){
    size_t nextIndex = 0;
    if(_emitter && !_emitterVirtualCore){
        emitterIndex = nextIndex;
        nextIndex = (nextIndex + 1) % _availableVirtualCores.size();
    }

    firstWorkerIndex = nextIndex;
    nextIndex = (nextIndex + _activeWorkers.size()) % _availableVirtualCores.size();

    if(_collector && !_collectorVirtualCore){
        collectorIndex = nextIndex;
    }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::mapNodesToVirtualCores(){
    size_t emitterIndex, firstWorkerIndex, collectorIndex;
    getMappingIndexes(emitterIndex, firstWorkerIndex, collectorIndex);

    //TODO: Che succede se la farm ha l'emitter di default? (non estende
    //      adaptive node e quindi la getThreadHandler non c'e' (fallisce lo static_cast prima)):(
    if(_emitter){
        if(!_emitterVirtualCore){
            _emitterVirtualCore = _availableVirtualCores.at(emitterIndex);
        }
        if(!_emitterVirtualCore->isHotPlugged()){
            _emitterVirtualCore->hotPlug();
        }
        _emitter->getThreadHandler()->move(_emitterVirtualCore);
    }

    for(size_t i = 0; i < _activeWorkers.size(); i++){
        topology::VirtualCore* vc = _availableVirtualCores.at((firstWorkerIndex + i) % _availableVirtualCores.size());
        _activeWorkersVirtualCores.push_back(vc);
        if(!vc->isHotPlugged()){
            vc->hotPlug();
        }
        _activeWorkers.at(i)->getThreadHandler()->move(vc);
    }

    if(_collector){
        if(!_collectorVirtualCore){
            _collectorVirtualCore = _availableVirtualCores.at(collectorIndex);
        }
        if(!_collectorVirtualCore->isHotPlugged()){
            _collectorVirtualCore->hotPlug();
        }
        _collector->getThreadHandler()->move(_collectorVirtualCore);
    }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::applyUnusedVirtualCoresStrategy(StrategyUnusedVirtualCores strategyUnused, const std::vector<topology::VirtualCore*>& unusedVirtualCores){
    switch(strategyUnused){
        case STRATEGY_UNUSED_VC_OFF:{
            for(size_t i = 0; i < unusedVirtualCores.size(); i++){
                topology::VirtualCore* vc = unusedVirtualCores.at(i);
                if(vc->isHotPluggable() && vc->isHotPlugged()){
                    vc->hotUnplug();
                }
            }
        }break;
        case STRATEGY_UNUSED_VC_LOWEST_FREQUENCY:{
            std::vector<cpufreq::Domain*> unusedDomains = _p->cpufreq->getDomainsComplete(unusedVirtualCores);
            for(size_t i = 0; i < unusedDomains.size(); i++){
                cpufreq::Domain* domain = unusedDomains.at(i);
                if(!domain->setGovernor(cpufreq::GOVERNOR_POWERSAVE)){
                    if(!domain->setGovernor(cpufreq::GOVERNOR_USERSPACE) ||
                       !domain->setLowestFrequencyUserspace()){
                        throw std::runtime_error("AdaptivityManagerFarm: Impossible to set lowest frequency "
                                                 "for unused virtual cores.");
                    }
                }
            }
        }break;
        default:{
            return;
        }
    }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::applyUnusedVirtualCoresStrategy(){
    /**
     * OFF 'includes' LOWEST_FREQUENCY. i.e. If we shutdown all the virtual cores
     * on a domain, we can also lower its frequency to the minimum.
     * TODO: Explain better
     */
    std::vector<topology::VirtualCore*> virtualCores;
    if(_p->strategyInactiveVirtualCores != STRATEGY_UNUSED_VC_NONE){
        utils::insertToEnd(_inactiveWorkersVirtualCores, virtualCores);
    }
    if(_p->strategyUnusedVirtualCores != STRATEGY_UNUSED_VC_NONE){
        utils::insertToEnd(_unusedVirtualCores, virtualCores);
    }
    applyUnusedVirtualCoresStrategy(STRATEGY_UNUSED_VC_LOWEST_FREQUENCY, virtualCores);


    virtualCores.clear();
    if(_p->strategyInactiveVirtualCores == STRATEGY_UNUSED_VC_OFF ||
       _p->strategyInactiveVirtualCores == STRATEGY_UNUSED_VC_AUTO){
        utils::insertToEnd(_inactiveWorkersVirtualCores, virtualCores);
    }
    if(_p->strategyUnusedVirtualCores == STRATEGY_UNUSED_VC_OFF ||
       _p->strategyUnusedVirtualCores == STRATEGY_UNUSED_VC_AUTO){
        utils::insertToEnd(_unusedVirtualCores, virtualCores);
    }
    applyUnusedVirtualCoresStrategy(STRATEGY_UNUSED_VC_AUTO, virtualCores);
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::updateScalableDomains(){
    std::vector<topology::VirtualCore*> frequencyScalableVirtualCores = _activeWorkersVirtualCores;
    /**
     * Node sensitivity may be not satisfied both because it was not requested, or because
     * it was requested but it was not possible to satisfy it. In both cases, we need to scale
     * the virtual core of the node as we scale the others.
     */
    if(_emitter && !_emitterSensitivitySatisfied){
        frequencyScalableVirtualCores.push_back(_emitterVirtualCore);
    }
    if(_collector && !_collectorSensitivitySatisfied){
        frequencyScalableVirtualCores.push_back(_collectorVirtualCore);
    }

    _scalableDomains = _p->cpufreq->getDomains(frequencyScalableVirtualCores);
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::updatePstate(cpufreq::Frequency frequency){
    updateScalableDomains();
    for(size_t i = 0; i < _scalableDomains.size(); i++){
        if(!_scalableDomains.at(i)->setGovernor(_p->frequencyGovernor)){
            throw std::runtime_error("AdaptivityManagerFarm: Impossible to set the specified governor.");
        }
        if(_p->frequencyGovernor != cpufreq::GOVERNOR_USERSPACE){
            if(!_scalableDomains.at(i)->setGovernorBounds(_p->frequencyLowerBound,
                                                          _p->frequencyUpperBound)){
                throw std::runtime_error("AdaptivityManagerFarm: Impossible to set the specified governor's bounds.");
            }
        }else if(_p->strategyFrequencies != STRATEGY_FREQUENCY_OS){
            if(!_scalableDomains.at(i)->setFrequencyUserspace(frequency)){
                throw std::runtime_error("AdaptivityManagerFarm: Impossible to set the specified frequency.");
            }
        }
    }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::mapAndSetFrequencies(){
    if(_p->strategyMapping == STRATEGY_MAPPING_NO){
        return;
    }

    manageSensitiveNodes();
    mapNodesToVirtualCores();
    for(size_t i = 0; i < _availableVirtualCores.size(); i++){
        topology::VirtualCore* vc = _availableVirtualCores.at(i);
        if(vc != _emitterVirtualCore && vc != _collectorVirtualCore &&
           !utils::contains(_activeWorkersVirtualCores, vc) &&
           !utils::contains(_inactiveWorkersVirtualCores, vc)){
            _unusedVirtualCores.push_back(vc);
        }
    }
    updateUsedCpus();
    applyUnusedVirtualCoresStrategy();

    if(_p->strategyFrequencies != STRATEGY_FREQUENCY_NO){
        if(_p->strategyFrequencies != STRATEGY_FREQUENCY_OS){
            /** We suppose that all the domains have the same available frequencies. **/
            _availableFrequencies = _p->cpufreq->getDomains().at(0)->getAvailableFrequencies();
            /** Sets the current frequency to the highest possible. **/
            _currentConfiguration.frequency = _availableFrequencies.back();
        }
        updatePstate(_currentConfiguration.frequency);
    }
}

template<typename lb_t, typename gt_t>
inline void AdaptivityManagerFarm<lb_t, gt_t>::updateMonitoredValues(){
    NodeSample sample;

    double workerAverageBandwidth;
    double workerAverageUtilization;

    _averageBandwidth = 0;
    _averageUtilization = 0;
    _usedJoules.zero();
    _unusedJoules.zero();

    uint numStoredSamples = (_elapsedSamples < _p->numSamples)?_elapsedSamples:_p->numSamples;
    /****************** Bandwidth and utilization ******************/
    for(size_t i = 0; i < _currentConfiguration.numWorkers; i++){
        workerAverageBandwidth = 0;
        workerAverageUtilization = 0;
        for(size_t j = 0; j < numStoredSamples; j++){
            sample = _nodeSamples.at(i).at(j);
            workerAverageBandwidth += sample.tasksCount;
            workerAverageUtilization += sample.loadPercentage;
        }
        _averageBandwidth += (workerAverageBandwidth / ((double) numStoredSamples * (double) _p->samplingInterval));
        _averageUtilization += (workerAverageUtilization / numStoredSamples);
    }
    _averageUtilization /= _currentConfiguration.numWorkers;

    /****************** Energy ******************/
    for(size_t i = 0; i < numStoredSamples; i++){
    	_usedJoules += _usedCpusEnergySamples.at(i);
    	_unusedJoules += _unusedCpusEnergySamples.at(i);
    }
    _usedJoules /= (double) numStoredSamples;
    _unusedJoules /= (double) numStoredSamples;
}

template<typename lb_t, typename gt_t>
inline bool AdaptivityManagerFarm<lb_t, gt_t>::isContractViolated() const{
    switch(_p->contractType){
        case CONTRACT_UTILIZATION:{
            return isContractViolated(_averageUtilization);
        }break;
        case CONTRACT_BANDWIDTH:
        case CONTRACT_COMPLETION_TIME:{
            return isContractViolated(_averageBandwidth);
        }break;
    }
    return false;
}

template<typename lb_t, typename gt_t>
inline bool AdaptivityManagerFarm<lb_t, gt_t>::isContractViolated(double monitoredValue) const{
    switch(_p->contractType){
        case CONTRACT_UTILIZATION:{
            return monitoredValue < _p->underloadThresholdFarm ||
                   monitoredValue > _p->overloadThresholdFarm;
        }break;
        case CONTRACT_BANDWIDTH:{
            double offset = (_p->requiredBandwidth * _p->maxBandwidthVariation) / 100.0;
            return monitoredValue < _p->requiredBandwidth ||
                   monitoredValue > _p->requiredBandwidth + offset;
        }break;
        case CONTRACT_COMPLETION_TIME:{
            return monitoredValue < _p->requiredBandwidth;
        }break;
    }
    return false;
}

template<typename lb_t, typename gt_t>
inline double AdaptivityManagerFarm<lb_t, gt_t>::getEstimatedMonitoredValue(const FarmConfiguration& configuration) const{
    double scalingFactor = 0;
    switch(_p->strategyFrequencies){
        case STRATEGY_FREQUENCY_NO:
        case STRATEGY_FREQUENCY_OS:{
            scalingFactor = (double) configuration.numWorkers /
                            (double) _currentConfiguration.numWorkers;
        }break;
        case STRATEGY_FREQUENCY_CORES_CONSERVATIVE:
        case STRATEGY_FREQUENCY_POWER_CONSERVATIVE:{
            scalingFactor = (double)(configuration.frequency * configuration.numWorkers) /
                            (double)(_currentConfiguration.frequency * _currentConfiguration.numWorkers);
        }break;
    }

    switch(_p->contractType){
        case CONTRACT_UTILIZATION:{
            return _averageUtilization * (1.0 / scalingFactor);
        }break;
        case CONTRACT_BANDWIDTH:
        case CONTRACT_COMPLETION_TIME:{
            return _averageBandwidth * scalingFactor;
        }break;
    }
    return 0;
}

template<typename lb_t, typename gt_t>
inline double AdaptivityManagerFarm<lb_t, gt_t>::getEstimatedConsumption(const FarmConfiguration& configuration) const{
    cpufreq::VoltageTableKey key(configuration.numWorkers, configuration.frequency);
    cpufreq::VoltageTableIterator it = _voltageTable.find(key);
    //TODO: Nettare rispetto al consumo quando l'applicazioine non gira
    if(it != _voltageTable.end()){
        cpufreq::Voltage v = it->second;
        double watts = configuration.numWorkers*configuration.frequency*v*v;
        if(_p->contractType == CONTRACT_COMPLETION_TIME){
            time_t now = time(NULL);
            if(_deadline > now){
                return watts * (_deadline - now);
            }else{
                return watts;
            }
        }else{
            return watts;
        }
    }else{
        throw std::runtime_error("Frequency and/or number of virtual cores not found in voltage table.");
    }
}

template<typename lb_t, typename gt_t>
inline bool AdaptivityManagerFarm<lb_t, gt_t>::isBestSuboptimalValue(double x, double y) const{
    double distanceX, distanceY;
    switch(_p->contractType){
        case CONTRACT_UTILIZATION:{
            // Concerning utilization factors, if both are suboptimal, we prefer the closest to the lower bound.
            distanceX = _p->underloadThresholdFarm - x;
            distanceY = _p->underloadThresholdFarm - y;
        }break;
        case CONTRACT_BANDWIDTH:
        case CONTRACT_COMPLETION_TIME:{
            // Concerning bandwidths, if both are suboptimal, we prefer the higher one.
            distanceX = x - _p->requiredBandwidth;
            distanceY = y - _p->requiredBandwidth;
        }break;
    }

    if(distanceX > 0 && distanceY < 0){
        return true;
    }else if(distanceX < 0 && distanceY > 0){
        return false;
    }else{
        return std::abs(distanceX) < std::abs(distanceY);
    }
}

template<typename lb_t, typename gt_t>
FarmConfiguration AdaptivityManagerFarm<lb_t, gt_t>::getNewConfiguration() const{
    FarmConfiguration r;
    double estimatedMonitoredValue = 0;
    double bestSuboptimalValue;
    switch(_p->contractType){
        case CONTRACT_UTILIZATION:{
            bestSuboptimalValue = _averageUtilization;
        }break;
        case CONTRACT_BANDWIDTH:
        case CONTRACT_COMPLETION_TIME:{
            bestSuboptimalValue = _averageBandwidth;
        }break;
    }

    FarmConfiguration bestSuboptimalConfiguration = _currentConfiguration;
    bool feasibleSolutionFound = false;

    switch(_p->strategyFrequencies){
        case STRATEGY_FREQUENCY_NO:
        case STRATEGY_FREQUENCY_OS:{
            for(size_t i = 1; i <= _maxNumWorkers; i++){
                FarmConfiguration examinedConfiguration(i);
                estimatedMonitoredValue = getEstimatedMonitoredValue(examinedConfiguration);
                if(!isContractViolated(estimatedMonitoredValue)){
                    feasibleSolutionFound = true;
                    return examinedConfiguration;
                }else if(!feasibleSolutionFound && isBestSuboptimalValue(estimatedMonitoredValue, bestSuboptimalValue)){
                    bestSuboptimalValue = estimatedMonitoredValue;
                    bestSuboptimalConfiguration.numWorkers = i;
                }
            }
        }break;
        case STRATEGY_FREQUENCY_CORES_CONSERVATIVE:{
            for(size_t i = 1; i <= _maxNumWorkers; i++){
                for(size_t j = 0; j < _availableFrequencies.size(); j++){
                    FarmConfiguration examinedConfiguration(i, _availableFrequencies.at(j));
                    estimatedMonitoredValue = getEstimatedMonitoredValue(examinedConfiguration);
                     if(!isContractViolated(estimatedMonitoredValue)){
                         feasibleSolutionFound = true;
                         return examinedConfiguration;
                     }else if(!feasibleSolutionFound && isBestSuboptimalValue(estimatedMonitoredValue, bestSuboptimalValue)){
                         bestSuboptimalValue = estimatedMonitoredValue;
                         bestSuboptimalConfiguration = examinedConfiguration;
                     }
                }
            }
        }break;
        case STRATEGY_FREQUENCY_POWER_CONSERVATIVE:{
            double minEstimatedPower = std::numeric_limits<double>::max();
            double estimatedPower = 0;
            for(size_t i = 1; i <= _maxNumWorkers; i++){
                for(size_t j = 0; j < _availableFrequencies.size(); j++){
                    FarmConfiguration examinedConfiguration(i, _availableFrequencies.at(j));
                    estimatedMonitoredValue = getEstimatedMonitoredValue(examinedConfiguration);
                    if(!isContractViolated(estimatedMonitoredValue)){
                        estimatedPower = getEstimatedConsumption(examinedConfiguration);
                        if(estimatedPower < minEstimatedPower){
                            minEstimatedPower = estimatedPower;
                            r = examinedConfiguration;
                            feasibleSolutionFound = true;
                        }
                    }else if(!feasibleSolutionFound && isBestSuboptimalValue(estimatedMonitoredValue, bestSuboptimalValue)){
                        bestSuboptimalValue = estimatedMonitoredValue;
                        bestSuboptimalConfiguration = examinedConfiguration;
                    }
                }
            }
        }break;
    }

    if(feasibleSolutionFound){
        return r;
    }else{
        return bestSuboptimalConfiguration;
    }
}


template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::updateUsedCpus(){

    _usedCpus.clear();
    _unusedCpus.clear();
    for(size_t i = 0; i < _activeWorkersVirtualCores.size(); i++){
        topology::CpuId cpuId = _activeWorkersVirtualCores.at(i)->getCpuId();
        if(!utils::contains(_usedCpus, cpuId)){
            _usedCpus.push_back(cpuId);
        }
    }
    if(_emitterVirtualCore){
        topology::CpuId cpuId = _emitterVirtualCore->getCpuId();
        if(!utils::contains(_usedCpus, cpuId)){
            _usedCpus.push_back(cpuId);
        }
    }
    if(_collectorVirtualCore){
        topology::CpuId cpuId = _collectorVirtualCore->getCpuId();
        if(!utils::contains(_usedCpus, cpuId)){
            _usedCpus.push_back(cpuId);
        }
    }

    std::vector<topology::Cpu*> cpus = _p->topology->getCpus();
    for(size_t i = 0; i < cpus.size(); i++){
        if(!utils::contains(_usedCpus, cpus.at(i)->getCpuId())){
            _unusedCpus.push_back(cpus.at(i)->getCpuId());
        }
    }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::changeConfiguration(FarmConfiguration configuration){
    if(!configuration.numWorkers){
        throw std::runtime_error("AdaptivityManagerFarm: fatal error, trying to activate zero "
                                 "workers.");
    }else if(configuration.numWorkers > _maxNumWorkers){
        throw std::runtime_error("AdaptivityManagerFarm: fatal error, trying to activate more "
                                 "workers than the maximum allowed.");
    }
    /****************** Workers change started ******************/
    if(_currentConfiguration.numWorkers != configuration.numWorkers){
        std::vector<cpufreq::RollbackPoint> rollbackPoints;
        if(_p->fastReconfiguration){
            for(size_t i = 0; i < _scalableDomains.size(); i++){
                rollbackPoints.push_back(_scalableDomains.at(i)->getRollbackPoint());
                setDomainToHighestFrequency(_scalableDomains.at(i));
            }
        }


        if(_currentConfiguration.numWorkers > configuration.numWorkers){
            /** Move workers from active to inactive. **/
            uint workersNumDiff = _currentConfiguration.numWorkers - configuration.numWorkers;
            utils::moveEndToFront(_activeWorkers, _inactiveWorkers, workersNumDiff);
            utils::moveEndToFront(_activeWorkersVirtualCores, _inactiveWorkersVirtualCores, workersNumDiff);
        }else{
            /** Move workers from inactive to active. **/
            /**
             * We need to map them again because if virtual cores were shutdown, the
             * threads that were running on them have been moved on different virtual
             * cores. Accordingly, when started again they would not be run on the
             * correct virtual cores.
             */
            uint workersNumDiff = configuration.numWorkers - _currentConfiguration.numWorkers;
            for(size_t i = 0; i < workersNumDiff; i++){
                topology::VirtualCore* vc = _inactiveWorkersVirtualCores.at(i);
                if(!vc->isHotPlugged()){
                    vc->hotPlug();
                }
                _inactiveWorkers.at(i)->getThreadHandler()->move(vc);
            }
            utils::moveFrontToEnd(_inactiveWorkers, _activeWorkers, workersNumDiff);
            utils::moveFrontToEnd(_inactiveWorkersVirtualCores, _activeWorkersVirtualCores, workersNumDiff);
        }

        updateUsedCpus();

        /** Stops farm. **/
        _emitter->produceNull();
        _farm->wait_freezing();
        /** Notify the nodes that a reconfiguration is happening. **/
        _emitter->notifyWorkersChange(_currentConfiguration.numWorkers, configuration.numWorkers);
        for(uint i = 0; i < configuration.numWorkers; i++){
            _activeWorkers.at(i)->notifyWorkersChange(_currentConfiguration.numWorkers, configuration.numWorkers);
        }
        if(_collector){
            _collector->notifyWorkersChange(_currentConfiguration.numWorkers, configuration.numWorkers);
        }
        /** Start the farm again. **/
        _farm->run_then_freeze(configuration.numWorkers);
        //TODO: Se la farm non è stata avviata con la run_then_freeze questo potrebbe essere un problema.
        if(_p->fastReconfiguration){
            _p->cpufreq->rollback(rollbackPoints);
        }
    }
    /****************** Workers change terminated ******************/
    applyUnusedVirtualCoresStrategy();
    /****************** P-state change started ******************/
    //TODO: Maybe sensitivity could not be satisfied with the maximum number of workers but could be satisfied now.
    if(_p->strategyFrequencies != STRATEGY_FREQUENCY_NO){
        updatePstate(configuration.frequency);
    }
    /****************** P-state change terminated ******************/
    _currentConfiguration = configuration;
}


template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::run(){
    /**
     * Wait for all the nodes to be running.
     */
    for(size_t i = 0; i < _activeWorkers.size(); i++){
        _activeWorkers.at(i)->waitThreadCreation();
    }
    if(_emitter){
        _emitter->waitThreadCreation();
    }
    if(_collector){
        _collector->waitThreadCreation();
    }

    if(_p->cpufreq->isBoostingSupported()){
        if(_p->turboBoost){
            _p->cpufreq->enableBoosting();
        }else{
            _p->cpufreq->disableBoosting();
        }
    }

    mapAndSetFrequencies();

    _nodeSamples.resize(_activeWorkers.size());
    _usedCpusEnergySamples.resize(_p->numSamples);
    _unusedCpusEnergySamples.resize(_p->numSamples);
    for(size_t i = 0; i < _activeWorkers.size(); i++){
        _nodeSamples.at(i).resize(_p->numSamples);
    }

    _p->energy->resetCountersCpu();

    size_t nextSampleIndex = 0;
    uint64_t samplesToDiscard = _p->samplesToDiscard;
    double lastOverheadMs = 0;
    double startOverheadMs = 0;
    double microsecsSleep = 0;
    if(_p->contractType == CONTRACT_COMPLETION_TIME){
        _remainingTasks = _p->expectedTasksNumber;
        _deadline = time(NULL) + _p->requiredCompletionTime;
    }
    while(!mustStop()){
        microsecsSleep = (double)_p->samplingInterval*(double)MAMMUT_MICROSECS_IN_SEC -
                         lastOverheadMs*(double)MAMMUT_MICROSECS_IN_MILLISEC;
        if(microsecsSleep < 0){
            microsecsSleep = 0;
        }
        usleep(microsecsSleep);

        startOverheadMs = utils::getMillisecondsTime();

        for(size_t i = 0; i < _currentConfiguration.numWorkers; i++){
            _activeWorkers.at(i)->askForSample();
        }
        for(size_t i = 0; i < _currentConfiguration.numWorkers; i++){
            NodeSample ns;
            bool workerRunning = _activeWorkers.at(i)->getSampleResponse(ns);
            if(!workerRunning){
                goto controlLoopEnd;
            }
            if(_p->contractType == CONTRACT_COMPLETION_TIME){
                if(_remainingTasks > ns.tasksCount){
                    _remainingTasks -= ns.tasksCount;
                }else{
                    _remainingTasks = 0;
                }
            }

            _nodeSamples.at(i).at(nextSampleIndex) = ns;
        }

        if(_p->contractType == CONTRACT_COMPLETION_TIME){
            time_t now = time(NULL);
            if(now >= _deadline){
                _p->requiredBandwidth = std::numeric_limits<double>::max();
            }else{
                _p->requiredBandwidth = _remainingTasks / (_deadline - now);
            }
        }

        _usedCpusEnergySamples.at(nextSampleIndex).zero();
        for(size_t i = 0; i < _usedCpus.size(); i++){
            energy::CounterCpu* currentCounter = _p->energy->getCounterCpu(_usedCpus.at(i));
            _usedCpusEnergySamples.at(nextSampleIndex) += currentCounter->getJoules();
        }
        _unusedCpusEnergySamples.at(nextSampleIndex).zero();
        for(size_t i = 0; i < _unusedCpus.size(); i++){
            energy::CounterCpu* currentCounter = _p->energy->getCounterCpu(_unusedCpus.at(i));
            _unusedCpusEnergySamples.at(nextSampleIndex) += currentCounter->getJoules();
        }
        _p->energy->resetCountersCpu();

        if(!samplesToDiscard){
            ++_elapsedSamples;
            nextSampleIndex = (nextSampleIndex + 1) % _p->numSamples;
            updateMonitoredValues();
        }else{
            --samplesToDiscard;
        }

        if(_p->observer){
            _p->observer->_numberOfWorkers = _currentConfiguration.numWorkers;
            _p->observer->_currentFrequency = _currentConfiguration.frequency;
            _p->observer->_emitterVirtualCore = _emitterVirtualCore;
            _p->observer->_workersVirtualCore = _activeWorkersVirtualCores;
            _p->observer->_collectorVirtualCore = _collectorVirtualCore;
            _p->observer->_currentBandwidth = _averageBandwidth;
            _p->observer->_currentUtilization = _averageUtilization;
            _p->observer->_usedJoules = _usedJoules;
            _p->observer->_unusedJoules = _unusedJoules;
            _p->observer->observe();
        }

        if((_elapsedSamples > _p->numSamples) && isContractViolated()){
            changeConfiguration(getNewConfiguration());
            _elapsedSamples = 0;
            nextSampleIndex = 0;
            samplesToDiscard = _p->samplesToDiscard;
        }

        lastOverheadMs = utils::getMillisecondsTime() - startOverheadMs;
    }
controlLoopEnd:
    ;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::stop(){
    _lock.lock();
    _stop = true;
    _lock.unlock();
}

template<typename lb_t, typename gt_t>
bool AdaptivityManagerFarm<lb_t, gt_t>::mustStop(){
    bool r;
    _lock.lock();
    r = _stop;
    _lock.unlock();
    return r;
}

}
}

#endif
