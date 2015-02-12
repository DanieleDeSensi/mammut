#ifndef MAMMUT_FASTFLOW_TPP_
#define MAMMUT_FASTFLOW_TPP_

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
    std::cout << "Run adaptive farm." << std::endl;
    return r;
}

template <typename lb_t, typename gt_t>
int AdaptiveFarm<lb_t, gt_t>::wait(){
    if(_adaptivityManager){
        _adaptivityManager->stop();
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
    _emitterSensitivitySatisfied(false),
    _collectorSensitivitySatisfied(false),
    _maxNumWorkers(_activeWorkers.size()),
    _currentConfiguration(_activeWorkers.size(), 0),
    _emitterVirtualCore(NULL),
    _collectorVirtualCore(NULL),
    _numRegisteredSamples(0){

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
               utils::appendFrontToEnd(_p->topology->virtualToPhysical(currentDomain->getVirtualCores()),
                                       physicalCoresInUnusedDomains);
           }
       }
    }
    return physicalCoresInUnusedDomains;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::setVirtualCoreToHighestFrequency(topology::VirtualCore* virtualCore){
    cpufreq::Domain* performanceDomain = _p->cpufreq->getDomain(virtualCore);
    bool frequencySet = performanceDomain->setGovernor(mammut::cpufreq::GOVERNOR_PERFORMANCE);
    if(!frequencySet){
        if(!performanceDomain->setGovernor(mammut::cpufreq::GOVERNOR_USERSPACE) ||
           !performanceDomain->setHighestFrequencyUserspace()){
            throw std::runtime_error("AdaptivityManagerFarm: Fatal error while setting highest frequency for "
                                     "sensitive emitter/collector. Try to run it without sensitivity parameters.");
        }
    }
}

template<typename lb_t, typename gt_t>
inline std::vector<topology::VirtualCore*> AdaptivityManagerFarm<lb_t, gt_t>::getAvailableVirtualCores(){
    std::vector<topology::VirtualCore*> r;
    switch(_p->strategyMapping){
        case STRATEGY_MAPPING_AUTO: //TODO: Auto should choose between all the others
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
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::manageSensitiveNodes(){
    if(_p->strategyFrequencies != STRATEGY_FREQUENCY_NO &&
      ((_p->sensitiveEmitter && !_emitterSensitivitySatisfied) ||
       (_p->sensitiveCollector && !_collectorSensitivitySatisfied))){
        size_t scalableVirtualCoresNum = _activeWorkers.size() +
                                         (_emitter && !_p->sensitiveEmitter) +
                                         (_collector && !_p->sensitiveCollector);
        /** When sensitive is specified, we always choose the WEC mapping. **/
        std::vector<topology::VirtualCore*> scalableVirtualCores(_unusedVirtualCores.begin(), (scalableVirtualCoresNum < _unusedVirtualCores.size())?
                                                                                               _unusedVirtualCores.begin() + scalableVirtualCoresNum:
                                                                                               _unusedVirtualCores.end());
        std::vector<topology::PhysicalCore*> performancePhysicalCores;
        performancePhysicalCores = getSeparatedDomainPhysicalCores(scalableVirtualCores);
        if(performancePhysicalCores.size()){
            size_t index = 0;

            if(_p->sensitiveEmitter && _emitter){
                topology::VirtualCore* vc = performancePhysicalCores.at(index)->getVirtualCore();
                setVirtualCoreToHighestFrequency(vc);
                _emitterVirtualCore = vc;
                _emitterSensitivitySatisfied = true;
                index = (index + 1) % performancePhysicalCores.size();
            }

            if(_p->sensitiveCollector && _collector){
                topology::VirtualCore* vc = performancePhysicalCores.at(index)->getVirtualCore();
                setVirtualCoreToHighestFrequency(vc);
                _collectorVirtualCore = vc;
                _collectorSensitivitySatisfied = true;
                index = (index + 1) % performancePhysicalCores.size();
            }
        }
    }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::mapNodesToVirtualCores(){
    //TODO: Better to map [w-w-w-w-w......-w-w-E-C], [E-w-w-w-....w-w-w-C] or
    //                    [E-C-w-w-w-.......-w-w]? (first and third are the same only if we have fully used CPUs)
    // Now EWC is always applied
    if(_emitter && !_emitterVirtualCore){
        _emitterVirtualCore = _unusedVirtualCores.front();
        _unusedVirtualCores.erase(_unusedVirtualCores.begin());
    }

    for(size_t i = 0; i < _activeWorkers.size(); i++){
        topology::VirtualCore* vc = _unusedVirtualCores.at(i % _unusedVirtualCores.size());
        _activeWorkersVirtualCores.push_back(vc);
    }
    _unusedVirtualCores.erase(_unusedVirtualCores.begin(), _unusedVirtualCores.begin() + std::min(_activeWorkers.size(), _unusedVirtualCores.size()));

    if(_collector && !_collectorVirtualCore){
        _collectorVirtualCore = _unusedVirtualCores.front(); //TODO: May be empty
        _unusedVirtualCores.erase(_unusedVirtualCores.begin());
    }

    /** Perform mapping. **/
    //TODO: Che succede se la farm ha l'emitter di default? (non estende adaptive node):(
    if(_emitter){
        _emitter->getThreadHandler()->move(_emitterVirtualCore);
    }

    if(_collector){
        _collector->getThreadHandler()->move(_collectorVirtualCore);
    }

    for(size_t i = 0; i < _activeWorkers.size(); i++){
        _activeWorkers.at(i)->getThreadHandler()->move(_activeWorkersVirtualCores.at(i));
    }
}

template<typename lb_t, typename gt_t>
StrategyUnusedVirtualCores AdaptivityManagerFarm<lb_t, gt_t>::
                           computeAutoUnusedVCStrategy(const std::vector<topology::VirtualCore*>& virtualCores){
    for(size_t i = 0; i < virtualCores.size(); i++){
        /** If at least one is hotpluggable we apply the VC_OFF strategy. **/
        if(virtualCores.at(i)->isHotPluggable()){
            return STRATEGY_UNUSED_VC_OFF;
        }
    }

    if((_p->cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_POWERSAVE) ||
        _p->cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_USERSPACE))){
        return STRATEGY_UNUSED_VC_LOWEST_FREQUENCY;
    }
    return STRATEGY_UNUSED_VC_NONE;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::applyUnusedVirtualCoresStrategy(StrategyUnusedVirtualCores strategyUnusedVirtualCores,
                                                                        const std::vector<topology::VirtualCore*>& virtualCores){
    switch(_p->strategyUnusedVirtualCores){
        case STRATEGY_UNUSED_VC_OFF:{
            for(size_t i = 0; i < virtualCores.size(); i++){
                topology::VirtualCore* vc = virtualCores.at(i);
                if(vc->isHotPluggable()){
                    vc->hotUnplug();
                }
            }
        }break;
        case STRATEGY_UNUSED_VC_LOWEST_FREQUENCY:{
            std::vector<cpufreq::Domain*> unusedDomains = _p->cpufreq->getDomainsComplete(virtualCores);
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
void AdaptivityManagerFarm<lb_t, gt_t>::updatePstate(cpufreq::Frequency frequency){
    /**
     *  We only change and set frequency to domains that contain at
     *  least one used virtual core.
     **/
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

    std::vector<cpufreq::Domain*> usedDomains = _p->cpufreq->getDomains(frequencyScalableVirtualCores);
    for(size_t i = 0; i < usedDomains.size(); i++){
        if(!usedDomains.at(i)->setGovernor(_p->frequencyGovernor)){
            throw std::runtime_error("AdaptivityManagerFarm: Impossible to set the specified governor.");
        }
        if(_p->frequencyGovernor != cpufreq::GOVERNOR_USERSPACE){
            if(!usedDomains.at(i)->setGovernorBounds(_p->frequencyLowerBound,
                                                 _p->frequencyUpperBound)){
                throw std::runtime_error("AdaptivityManagerFarm: Impossible to set the specified governor's bounds.");
            }
        }else if(_p->strategyFrequencies != STRATEGY_FREQUENCY_OS){
            if(!usedDomains.at(i)->setFrequencyUserspace(frequency)){
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

    _unusedVirtualCores = getAvailableVirtualCores();
    manageSensitiveNodes();
    mapNodesToVirtualCores();

    if(_p->strategyUnusedVirtualCores == STRATEGY_UNUSED_VC_AUTO){
        _p->strategyUnusedVirtualCores = computeAutoUnusedVCStrategy(_unusedVirtualCores);
    }
    applyUnusedVirtualCoresStrategy(_p->strategyUnusedVirtualCores,
                                    _unusedVirtualCores);

    if(_p->strategyFrequencies != STRATEGY_FREQUENCY_NO && _p->strategyFrequencies != STRATEGY_FREQUENCY_OS){
        /** We suppose that all the domains have the same available frequencies. **/
        _availableFrequencies = _p->cpufreq->getDomains().at(0)->getAvailableFrequencies();
        _currentConfiguration.frequency = _availableFrequencies.back(); // Sets the current frequency to the highest possible.
        updatePstate(_currentConfiguration.frequency);
    }
}

template<typename lb_t, typename gt_t>
inline double AdaptivityManagerFarm<lb_t, gt_t>::getWorkerAverageLoad(size_t workerId){
    double r = 0;
    uint n = _numRegisteredSamples < _p->numSamples?_numRegisteredSamples:_p->numSamples;
    for(size_t i = 0; i < n; i++){
        r += _nodeSamples.at(workerId).at(i).loadPercentage;
    }
    return r / ((double) n);
}

template<typename lb_t, typename gt_t>
inline double AdaptivityManagerFarm<lb_t, gt_t>::getFarmAverageLoad(){
    double r = 0;
    for(size_t i = 0; i < _currentConfiguration.numWorkers; i++){
        r += getWorkerAverageLoad(i);
    }
    return r / _currentConfiguration.numWorkers;
}

template<typename lb_t, typename gt_t>
inline double AdaptivityManagerFarm<lb_t, gt_t>::getWorkerAverageBandwidth(size_t workerId){
    double r = 0;
    uint n = _numRegisteredSamples < _p->numSamples?_numRegisteredSamples:_p->numSamples;
    for(size_t i = 0; i < n; i++){
        r += _nodeSamples.at(workerId).at(i).tasksCount;
    }
    return r / ((double) n * (double) _p->samplingInterval);
}

template<typename lb_t, typename gt_t>
inline double AdaptivityManagerFarm<lb_t, gt_t>::getFarmAverageBandwidth(){
    double r = 0;
    for(size_t i = 0; i < _currentConfiguration.numWorkers; i++){
        r += getWorkerAverageBandwidth(i);
    }
    return r;
}

template<typename lb_t, typename gt_t>
inline double AdaptivityManagerFarm<lb_t, gt_t>::getMonitoredValue(){
    if(_p->requiredBandwidth){
        return getFarmAverageBandwidth();
    }else{
        return getFarmAverageLoad();
    }
}

template<typename lb_t, typename gt_t>
inline bool AdaptivityManagerFarm<lb_t, gt_t>::isContractViolated(double monitoredValue) const{
    if(_p->requiredBandwidth){
        double offset = (_p->requiredBandwidth * _p->maxBandwidthVariation) / 100.0;
        return monitoredValue < _p->requiredBandwidth - offset ||
               monitoredValue > _p->requiredBandwidth + offset;
    }else{
        return monitoredValue < _p->underloadThresholdFarm ||
               monitoredValue > _p->overloadThresholdFarm;
    }
}

template<typename lb_t, typename gt_t>
inline double AdaptivityManagerFarm<lb_t, gt_t>::getEstimatedMonitoredValue(double monitoredValue, const FarmConfiguration& configuration) const{
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

    if(_p->requiredBandwidth){
        return monitoredValue * scalingFactor;
    }else{
        return monitoredValue * (1.0 / scalingFactor);
    }
}

template<typename lb_t, typename gt_t>
inline double AdaptivityManagerFarm<lb_t, gt_t>::getEstimatedPower(const FarmConfiguration& configuration) const{
    cpufreq::VoltageTableKey key(configuration.numWorkers, configuration.frequency);
    cpufreq::VoltageTableIterator it = _voltageTable.find(key);
    if(it != _voltageTable.end()){
        cpufreq::Voltage v = it->second;
        return configuration.numWorkers*configuration.frequency*v*v;
    }else{
        throw std::runtime_error("Frequency and/or number of virtual cores not found in voltage table.");
    }
}

template<typename lb_t, typename gt_t>
inline double AdaptivityManagerFarm<lb_t, gt_t>::getImpossibleMonitoredValue() const{
    if(_p->requiredBandwidth){
        return -1;
    }else{
        return -1;
    }
}

template<typename lb_t, typename gt_t>
inline bool AdaptivityManagerFarm<lb_t, gt_t>::isBestSuboptimalValue(double x, double y) const{
    double distanceX, distanceY;
    if(_p->requiredBandwidth){
        // Concerning bandwidths, if both are suboptimal, we prefer the higher one.
        distanceX = x - _p->requiredBandwidth;
        distanceY = y - _p->requiredBandwidth;
    }else{
        // Concerning utilization factors, if both are suboptimal, we prefer the closest to the lower bound.
        distanceX = _p->underloadThresholdFarm - x;
        distanceY = _p->underloadThresholdFarm - y;
    }

    if(distanceX > 0 && distanceY < 0){
        return true;
    }else if(distanceX < 0 && distanceY > 0){
        return false;
    }else{
        return distanceX < distanceY;
    }
}

template<typename lb_t, typename gt_t>
FarmConfiguration AdaptivityManagerFarm<lb_t, gt_t>::getNewConfiguration(double monitoredValue) const{
    FarmConfiguration r;
    double estimatedMonitoredValue = 0;
    double bestSuboptimalValue = getImpossibleMonitoredValue();
    FarmConfiguration bestSuboptimalConfiguration;
    bool feasibleSolutionFound = false;

    switch(_p->strategyFrequencies){
        case STRATEGY_FREQUENCY_NO:
        case STRATEGY_FREQUENCY_OS:{
            for(size_t i = 0; i < _maxNumWorkers; i++){
                FarmConfiguration currentConfiguration(i);
                estimatedMonitoredValue = getEstimatedMonitoredValue(monitoredValue, currentConfiguration);
                if(!isContractViolated(estimatedMonitoredValue)){
                    feasibleSolutionFound = true;
                    return currentConfiguration;
                }else if(!feasibleSolutionFound && isBestSuboptimalValue(estimatedMonitoredValue, bestSuboptimalValue)){
                    bestSuboptimalValue = estimatedMonitoredValue;
                    bestSuboptimalConfiguration.numWorkers = i;
                }
            }
        }break;
        case STRATEGY_FREQUENCY_CORES_CONSERVATIVE:{
            for(size_t i = 0; i < _maxNumWorkers; i++){
                for(size_t j = 0; j < _availableFrequencies.size(); j++){
                    FarmConfiguration currentConfiguration(i, _availableFrequencies.at(j));
                    estimatedMonitoredValue = getEstimatedMonitoredValue(monitoredValue, currentConfiguration);
                     if(!isContractViolated(estimatedMonitoredValue)){
                         feasibleSolutionFound = true;
                         return currentConfiguration;
                     }else if(!feasibleSolutionFound && isBestSuboptimalValue(estimatedMonitoredValue, bestSuboptimalValue)){
                         bestSuboptimalValue = estimatedMonitoredValue;
                         bestSuboptimalConfiguration = currentConfiguration;
                     }
                }
            }
        }break;
        case STRATEGY_FREQUENCY_POWER_CONSERVATIVE:{
            double minEstimatedPower = std::numeric_limits<double>::max();
            double estimatedPower = 0;
            for(size_t i = 0; i < _maxNumWorkers; i++){
                for(size_t j = 0; j < _availableFrequencies.size(); j++){
                    FarmConfiguration currentConfiguration(i, _availableFrequencies.at(j));
                    estimatedMonitoredValue = getEstimatedMonitoredValue(monitoredValue, currentConfiguration);
                    if(!isContractViolated(estimatedMonitoredValue)){
                        estimatedPower = getEstimatedPower(currentConfiguration);
                        if(estimatedPower < minEstimatedPower){
                            minEstimatedPower = estimatedPower;
                            r = currentConfiguration;
                            feasibleSolutionFound = true;
                        }
                    }else if(!feasibleSolutionFound && isBestSuboptimalValue(estimatedMonitoredValue, bestSuboptimalValue)){
                        bestSuboptimalValue = estimatedMonitoredValue;
                        bestSuboptimalConfiguration = currentConfiguration;
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
void AdaptivityManagerFarm<lb_t, gt_t>::changeConfiguration(FarmConfiguration configuration){
    if(configuration.numWorkers > _maxNumWorkers){
        throw std::runtime_error("AdaptivityManagerFarm: fatal error, trying to activate more workers than the maximum allowed.");
    }
    /****************** Workers change started ******************/
    if(_currentConfiguration.numWorkers != configuration.numWorkers){
        if(_p->fastReconfiguration){
            ;//TODO:
        }
        _emitter->produceNull();
        _farm->wait_freezing();
        /** Notify the nodes that a reconfiguration is happening. **/
        _emitter->notifyWorkersChange(_currentConfiguration.numWorkers, configuration.numWorkers);
        for(uint i = 0; i < _currentConfiguration.numWorkers; i++){
            _activeWorkers.at(i)->notifyWorkersChange(_currentConfiguration.numWorkers, configuration.numWorkers);
        }
        if(_collector){
            _collector->notifyWorkersChange(_currentConfiguration.numWorkers, configuration.numWorkers);
        }
        /** Start the farm again. **/
        _farm->run_then_freeze(configuration.numWorkers);
        //TODO: Se la farm non è stata avviata con la run_then_freeze questo potrebbe essere un problema.
        uint workersNumDiff = std::abs(configuration.numWorkers - _currentConfiguration.numWorkers);
        if(_currentConfiguration.numWorkers > configuration.numWorkers){
            /** Move workers from active to inactive. **/
            utils::moveEndToFront(_activeWorkers, _inactiveWorkers, workersNumDiff);
            utils::moveEndToFront(_activeWorkersVirtualCores, _inactiveWorkersVirtualCores, workersNumDiff);
        }else{
            /** Move workers from inactive to active. **/
            utils::moveFrontToEnd(_inactiveWorkers, _activeWorkers, workersNumDiff);
            utils::moveFrontToEnd(_inactiveWorkersVirtualCores, _activeWorkersVirtualCores, workersNumDiff);
        }
    }
    /****************** Workers change terminated ******************/
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

    double x = utils::getMillisecondsTime();
    if(_p->cpufreq->isBoostingSupported()){
        if(_p->turboBoost){
            _p->cpufreq->enableBoosting();
        }else{
            _p->cpufreq->disableBoosting();
        }
    }

    mapAndSetFrequencies();
    std::cout << "Milliseconds elapsed: " << utils::getMillisecondsTime() - x << std::endl;

    std::cout << "Emitter node: " << _emitterVirtualCore->getVirtualCoreId() << std::endl;
    std::cout << "Collector node: " << _collectorVirtualCore->getVirtualCoreId() << std::endl;
    std::cout << "Worker nodes: ";
    for(size_t i = 0; i < _activeWorkers.size(); i++){
        std::cout <<  _activeWorkersVirtualCores.at(i)->getVirtualCoreId() << ", ";
    }
    std::cout << std::endl;

    _nodeSamples.resize(_activeWorkers.size());
    for(size_t i = 0; i < _activeWorkers.size(); i++){
        _nodeSamples.at(i).resize(_p->numSamples);
    }

    _lock.lock();
    size_t nextSampleIndex = 0;
    size_t maxNeededRegisteredSamples = std::max(_p->stabilizationSamples, _p->numSamples);
    while(!_stop){
        _lock.unlock();
        std::cout << "Manager running" << std::endl;
        sleep(_p->samplingInterval);
        for(size_t i = 0; i < _currentConfiguration.numWorkers; i++){
            _nodeSamples.at(i).at(nextSampleIndex) = _activeWorkers.at(i)->getAndResetSample();
        }
        nextSampleIndex = (nextSampleIndex + 1) % _p->numSamples;

        if(_numRegisteredSamples < maxNeededRegisteredSamples){
            ++_numRegisteredSamples;
        }

        if(_numRegisteredSamples >= _p->stabilizationSamples){
            double monitoredValue = getMonitoredValue();
            if(isContractViolated(monitoredValue)){
                FarmConfiguration newConfiguration = getNewConfiguration(monitoredValue);
                changeConfiguration(newConfiguration);
                _numRegisteredSamples = 0;
                nextSampleIndex = 0;
            }
        }

        _lock.lock();
    }
    _lock.unlock();
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::stop(){
    _lock.lock();
    _stop = true;
    _lock.unlock();
}

}
}

#endif
