#ifndef MAMMUT_FASTFLOW_TPP_
#define MAMMUT_FASTFLOW_TPP_

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
    _farm(farm),
    _adaptivityParameters(adaptivityParameters),
    _workers(_farm->getAdaptiveWorkers()),
    _activeWorkers(_workers.size()),
    _stop(false),
    _emitterVirtualCore(NULL),
    _collectorVirtualCore(NULL){
    ;
}

template<typename lb_t, typename gt_t>
AdaptivityManagerFarm<lb_t, gt_t>::~AdaptivityManagerFarm(){
    ;
}

template<typename lb_t, typename gt_t>
std::vector<topology::PhysicalCore*> AdaptivityManagerFarm<lb_t, gt_t>::getSeparatedDomainPhysicalCores(const std::vector<topology::VirtualCore*>& virtualCores) const{
    std::vector<cpufreq::Domain*> allDomains = _adaptivityParameters->cpufreq->getDomains();
    std::vector<cpufreq::Domain*> hypotheticWorkersDomains = _adaptivityParameters->cpufreq->getDomains(virtualCores);
    std::vector<topology::PhysicalCore*> physicalCoresInUnusedDomains;
    if(allDomains.size() > hypotheticWorkersDomains.size()){
       for(size_t i = 0; i < allDomains.size(); i++){
           cpufreq::Domain* currentDomain = allDomains.at(i);
           if(!utils::contains(hypotheticWorkersDomains, currentDomain)){
               utils::append(physicalCoresInUnusedDomains,
                             _adaptivityParameters->topology->virtualToPhysical(currentDomain->getVirtualCores()));
           }
       }
    }
    return physicalCoresInUnusedDomains;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::setVirtualCoreToHighestFrequency(topology::VirtualCore* virtualCore){
    cpufreq::Domain* performanceDomain = _adaptivityParameters->cpufreq->getDomain(virtualCore);
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
void AdaptivityManagerFarm<lb_t, gt_t>::setUnusedVirtualCores(){
    switch(_adaptivityParameters->strategyMapping){
        case STRATEGY_MAPPING_AUTO: //TODO: Auto should choose between all the others
        case STRATEGY_MAPPING_LINEAR:{
           /*
            * Generates a vector of virtual cores to be used for linear mapping.node
            * It contains first one virtual core per physical core (virtual cores
            * on the same CPU are consecutive).
            * Then, the other groups of virtual cores follow.
            */
            std::vector<topology::Cpu*> cpus = _adaptivityParameters->topology->getCpus();

            size_t virtualUsed = 0;
            size_t virtualPerPhysical = _adaptivityParameters->topology->getVirtualCores().size() /
                                        _adaptivityParameters->topology->getPhysicalCores().size();
            while(virtualUsed < virtualPerPhysical){
                for(size_t i = 0; i < cpus.size(); i++){
                    std::vector<topology::PhysicalCore*> physicalCores = cpus.at(i)->getPhysicalCores();
                    for(size_t j = 0; j < physicalCores.size(); j++){
                        _unusedVirtualCores.push_back(physicalCores.at(j)->getVirtualCores().at(virtualUsed));
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
StrategyUnusedVirtualCores AdaptivityManagerFarm<lb_t, gt_t>::
                           computeAutoUnusedVCStrategy(const std::vector<topology::VirtualCore*>& virtualCores){
    for(size_t i = 0; i < virtualCores.size(); i++){
        /** If at least one is hotpluggable we apply the VC_OFF strategy. **/
        if(virtualCores.at(i)->isHotPluggable()){
            return STRATEGY_UNUSED_VC_OFF;
        }
    }

    if((_adaptivityParameters->cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_POWERSAVE) ||
        _adaptivityParameters->cpufreq->isGovernorAvailable(cpufreq::GOVERNOR_USERSPACE))){
        return STRATEGY_UNUSED_VC_LOWEST_FREQUENCY;
    }
    return STRATEGY_UNUSED_VC_NONE;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::applyUnusedVirtualCoresStrategy(StrategyUnusedVirtualCores strategyUnusedVirtualCores,
                                                                        const std::vector<topology::VirtualCore*>& virtualCores){
    switch(_adaptivityParameters->strategyNeverUsedVirtualCores){
        case STRATEGY_UNUSED_VC_OFF:{
            for(size_t i = 0; i < virtualCores.size(); i++){
                topology::VirtualCore* vc = virtualCores.at(i);
                if(vc->isHotPluggable()){
                    vc->hotUnplug();
                }
            }
        }break;
        case STRATEGY_UNUSED_VC_LOWEST_FREQUENCY:{
            std::vector<cpufreq::Domain*> unusedDomains = _adaptivityParameters->cpufreq->getDomainsComplete(virtualCores);
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
void AdaptivityManagerFarm<lb_t, gt_t>::updatePstate(const std::vector<topology::VirtualCore*>& virtualCores,
                                                     cpufreq::Frequency frequency){
    /**
     *  We only change and set frequency to domains that contain at
     *  least one used virtual core.
     **/
    std::vector<cpufreq::Domain*> usedDomains = _adaptivityParameters->cpufreq->getDomains(virtualCores);

    for(size_t i = 0; i < usedDomains.size(); i++){
        if(!usedDomains.at(i)->setGovernor(_adaptivityParameters->frequencyGovernor)){
            throw std::runtime_error("AdaptivityManagerFarm: Impossible to set the specified governor.");
        }
        if(_adaptivityParameters->frequencyGovernor != cpufreq::GOVERNOR_USERSPACE){
            if(!usedDomains.at(i)->setGovernorBounds(_adaptivityParameters->frequencyLowerBound,
                                                 _adaptivityParameters->frequencyUpperBound)){
                throw std::runtime_error("AdaptivityManagerFarm: Impossible to set the specified governor's bounds.");;
            }
        }else if(_adaptivityParameters->strategyFrequencies != STRATEGY_FREQUENCY_OS){
            if(!usedDomains.at(i)->setFrequencyUserspace(frequency)){
                throw std::runtime_error("AdaptivityManagerFarm: Impossible to set the specified frequency.");;
            }
        }
    }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::mapAndSetFrequencies(){
    if(_adaptivityParameters->strategyMapping == STRATEGY_MAPPING_NO){
        return;
    }
    /** Computes map. **/
    setUnusedVirtualCores();

    bool emitterMappingRequired = (_farm->getEmitter()!=NULL);
    bool collectorMappingRequired = (_farm->getCollector()!=NULL);
    std::vector<topology::VirtualCore*> frequencyScalableVirtualCores;

    /**
     * If requested, and if there are available domains, run emitter or collector (or both) at
     * the highest frequency.
     */
    if(_adaptivityParameters->strategyFrequencies != STRATEGY_FREQUENCY_NO &&
      (_adaptivityParameters->sensitiveEmitter || _adaptivityParameters->sensitiveCollector)){
        size_t scalableVirtualCoresNum = _workers.size() +
                                      (emitterMappingRequired && !_adaptivityParameters->sensitiveEmitter)?1:0 +
                                      (collectorMappingRequired && !_adaptivityParameters->sensitiveCollector)?1:0;
        /** When sensitive is specified, we always choose the WEC mapping. **/
        std::vector<topology::VirtualCore*> scalableVirtualCores(_unusedVirtualCores.begin(), (scalableVirtualCoresNum < _unusedVirtualCores.size())?
                                                                                                   _unusedVirtualCores.begin() + scalableVirtualCoresNum:
                                                                                                   _unusedVirtualCores.end());
        std::vector<topology::PhysicalCore*> performancePhysicalCores = getSeparatedDomainPhysicalCores(scalableVirtualCores);
        if(performancePhysicalCores.size()){
            size_t index = 0;

            if(_adaptivityParameters->sensitiveEmitter && emitterMappingRequired){
                topology::VirtualCore* vc = performancePhysicalCores.at(index)->getVirtualCore();
                setVirtualCoreToHighestFrequency(vc);
                _emitterVirtualCore = vc;
                emitterMappingRequired = false;
                index = (index + 1) % performancePhysicalCores.size();
            }

            if(_adaptivityParameters->sensitiveCollector && collectorMappingRequired){
                topology::VirtualCore* vc = performancePhysicalCores.at(index)->getVirtualCore();
                setVirtualCoreToHighestFrequency(vc);
                _collectorVirtualCore = vc;
                collectorMappingRequired = false;
                index = (index + 1) % performancePhysicalCores.size();
            }
        }
    }

    //TODO: Better to map [w-w-w-w-w......-w-w-E-C], [E-w-w-w-....w-w-w-C] or
    //                    [E-C-w-w-w-.......-w-w]? (first and third are the same only if we have fully used CPUs)
    // Now EWC is always applied
    if(emitterMappingRequired){
        _emitterVirtualCore = _unusedVirtualCores.front();
        _unusedVirtualCores.erase(_unusedVirtualCores.begin());
        frequencyScalableVirtualCores.push_back(_emitterVirtualCore);
    }

    for(size_t i = 0; i < _workers.size(); i++){
        topology::VirtualCore* vc = _unusedVirtualCores.at(i);
        _workersVirtualCores.push_back(vc);
        frequencyScalableVirtualCores.push_back(vc);
    }
    _unusedVirtualCores.erase(_unusedVirtualCores.begin(), _unusedVirtualCores.begin() + _workers.size());

    if(collectorMappingRequired){
        _collectorVirtualCore = _unusedVirtualCores.front();
        _unusedVirtualCores.erase(_unusedVirtualCores.begin());
        frequencyScalableVirtualCores.push_back(_collectorVirtualCore);
    }

    /** Perform mapping. **/
    AdaptiveNode* node = NULL;
    //TODO: Che succede se la farm ha l'emitter di default? :(
    if((node = _farm->getAdaptiveEmitter())){
        node->getThreadHandler()->move(_emitterVirtualCore);
    }

    if((node = _farm->getAdaptiveCollector())){
        node->getThreadHandler()->move(_collectorVirtualCore);
    }

    for(size_t i = 0; i < _workers.size(); i++){
        _workers.at(i)->getThreadHandler()->move(_workersVirtualCores.at(i));
    }

    if(_adaptivityParameters->strategyNeverUsedVirtualCores == STRATEGY_UNUSED_VC_AUTO){
        _adaptivityParameters->strategyNeverUsedVirtualCores = computeAutoUnusedVCStrategy(_unusedVirtualCores);
    }
    applyUnusedVirtualCoresStrategy(_adaptivityParameters->strategyNeverUsedVirtualCores,
                                    _unusedVirtualCores);

    if(_adaptivityParameters->strategyFrequencies != STRATEGY_FREQUENCY_NO){
        cpufreq::Frequency highestFrequency = _adaptivityParameters->cpufreq->getDomains().at(0)->getAvailableFrequencies().back();
        updatePstate(frequencyScalableVirtualCores, highestFrequency);
    }
}

template<typename lb_t, typename gt_t>
double AdaptivityManagerFarm<lb_t, gt_t>::getWorkerAverageLoad(size_t workerId){
    double r = 0;
    for(size_t i = 0; i < _adaptivityParameters->numSamples; i++){
        r += _loadSamples.at(workerId).at(i);
    }
    return r/(double) _adaptivityParameters->numSamples;
}

template<typename lb_t, typename gt_t>
double AdaptivityManagerFarm<lb_t, gt_t>::getFarmAverageLoad(){
    double r = 0;
    for(size_t i = 0; i < _activeWorkers; i++){
        r += getWorkerAverageLoad(i);
    }
    return r / _activeWorkers;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::run(){
    /**
     * Wait for all the nodes to be running.
     */
    std::vector<AdaptiveNode*> adaptiveWorkers = _farm->getAdaptiveWorkers();
    AdaptiveNode* adaptiveEmitter = _farm->getAdaptiveEmitter();
    AdaptiveNode* adaptiveCollector = _farm->getAdaptiveCollector();
    for(size_t i = 0; i < adaptiveWorkers.size(); i++){
        adaptiveWorkers.at(i)->waitThreadCreation();
    }
    if(adaptiveEmitter){
        adaptiveEmitter->waitThreadCreation();
    }
    if(adaptiveCollector){
        adaptiveCollector->waitThreadCreation();
    }

    double x = utils::getMillisecondsTime();
    mapAndSetFrequencies();
    std::cout << "Milliseconds elapsed: " << utils::getMillisecondsTime() - x << std::endl;

    std::cout << "Emitter node: " << _emitterVirtualCore->getVirtualCoreId() << std::endl;
    std::cout << "Collector node: " << _collectorVirtualCore->getVirtualCoreId() << std::endl;
    std::cout << "Worker nodes: ";
    for(size_t i = 0; i < adaptiveWorkers.size(); i++){
        std::cout <<  _workersVirtualCores.at(i)->getVirtualCoreId() << ", ";
    }
    std::cout << std::endl;

    _loadSamples.resize(_workers.size());
    for(size_t i = 0; i < _workers.size(); i++){
        _loadSamples.at(i).resize(_adaptivityParameters->numSamples, 0);
    }

    _lock.lock();
    size_t nextSampleIndex = 0;
    while(!_stop){
        _lock.unlock();
        std::cout << "Manager running" << std::endl;
        sleep(_adaptivityParameters->samplingInterval);
        for(size_t i = 0; i < _activeWorkers; i++){
            double workPercentage;
            uint64_t taskCount;
            _workers.at(i)->getAndResetStatistics(workPercentage, taskCount);
            _loadSamples.at(i).at(nextSampleIndex) = workPercentage;
            std::cout << "tt" << taskCount << std::endl;
        }
        ++nextSampleIndex;

        double avgLoad = getFarmAverageLoad();

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
