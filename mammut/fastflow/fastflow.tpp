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
    _stop(false),
    _firstWorkerIndex(0),
    _emitterIndex(0),
    _collectorIndex(0){
    ;
}

template<typename lb_t, typename gt_t>
AdaptivityManagerFarm<lb_t, gt_t>::~AdaptivityManagerFarm(){
    ;
}

template<typename lb_t, typename gt_t>
std::vector<topology::PhysicalCore*> AdaptivityManagerFarm<lb_t, gt_t>::findPossiblePerformancePhysicalCores(const std::vector<topology::VirtualCore*>& workersVirtualCores) const{
    std::vector<cpufreq::Domain*> allDomains = _adaptivityParameters->cpufreq->getDomains();
    std::vector<cpufreq::Domain*> hypotheticWorkersDomains = _adaptivityParameters->cpufreq->getDomains(workersVirtualCores);
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
size_t AdaptivityManagerFarm<lb_t, gt_t>::getVirtualCoreIndex(topology::VirtualCore* virtualCore){
    size_t index = std::find(_availableVirtualCores.begin(), _availableVirtualCores.end(), virtualCore) - _availableVirtualCores.begin();
    if(index >= _availableVirtualCores.size()){
        throw std::runtime_error("setFarmNodeToHighestFrequency: Fatal error. Virtual core not found.");
    }
    return index;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::setVirtualCoreToHighestFrequency(topology::VirtualCore* virtualCore){
    cpufreq::Domain* performanceDomain = _adaptivityParameters->cpufreq->getDomain(virtualCore);
    bool frequencySet = performanceDomain->setGovernor(mammut::cpufreq::GOVERNOR_PERFORMANCE);
    if(!frequencySet){
        performanceDomain->setGovernor(mammut::cpufreq::GOVERNOR_USERSPACE);
        if(!performanceDomain->setHighestFrequencyUserspace()){
            throw std::runtime_error("AdaptivityManagerFarm: Fatal error while setting highest frequency for "
                                     "sensitive emitter/collector. Try to run it without sensitivity parameters.");
        }
    }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::mapAndSetFrequencies(){
    switch(_adaptivityParameters->strategyMapping){
        case STRATEGY_MAPPING_OS:{
            return;
        }break;
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
                        _availableVirtualCores.push_back(physicalCores.at(j)->getVirtualCores().at(virtualUsed));
                    }
                }
                ++virtualUsed;
            }
        }break;
        case STRATEGY_MAPPING_CACHE_EFFICIENT:{
            throw std::runtime_error("Not yet supported.");
        }
    }

    /** Computes map. **/
    bool emitterMappingRequired = (_farm->getEmitter()!=NULL);
    bool collectorMappingRequired = (_farm->getCollector()!=NULL);
    std::vector<topology::VirtualCore*> frequencyScalableVirtualCores;

    /**
     * If requested, and if there are available domains, run emitter or collector (or both) at
     * the highest frequency.
     */
    if(_adaptivityParameters->strategyFrequencies != STRATEGY_FREQUENCY_NO &&
      (_adaptivityParameters->sensitiveEmitter || _adaptivityParameters->sensitiveCollector)){
        size_t scalableVirtualCores = _workers.size() +
                                      (emitterMappingRequired && !_adaptivityParameters->sensitiveEmitter)?1:0 +
                                      (collectorMappingRequired && !_adaptivityParameters->sensitiveCollector)?1:0;
        /** When sensitive is specified, we always choose the WEC mapping. **/
        std::vector<topology::VirtualCore*> workersVirtualCores(_availableVirtualCores.begin(), (scalableVirtualCores < _availableVirtualCores.size())?
                                                                                                   _availableVirtualCores.begin() + scalableVirtualCores:
                                                                                                   _availableVirtualCores.end());
        std::vector<topology::PhysicalCore*> performancePhysicalCores = findPossiblePerformancePhysicalCores(workersVirtualCores);
        if(performancePhysicalCores.size()){
            size_t index = 0;
            cpufreq::Domain *emitterDomain = NULL, *collectorDomain = NULL;

            if(_adaptivityParameters->sensitiveEmitter && emitterMappingRequired){
                topology::VirtualCore* vc = performancePhysicalCores.at(index)->getVirtualCore();
                setVirtualCoreToHighestFrequency(vc);
                _emitterIndex = getVirtualCoreIndex(vc);
                emitterMappingRequired = false;
                index = (index + 1) % performancePhysicalCores.size();
            }

            if(_adaptivityParameters->sensitiveCollector && collectorMappingRequired){
                topology::VirtualCore* vc = performancePhysicalCores.at(index)->getVirtualCore();
                setVirtualCoreToHighestFrequency(vc);
                _collectorIndex = getVirtualCoreIndex(vc);
                collectorMappingRequired = false;
                index = (index + 1) % performancePhysicalCores.size();
            }
        }
    }


    /** Indexes computation. **/
    if(_availableVirtualCores.size()){
        _firstWorkerIndex = 0;
        //TODO: Better to map [w-w-w-w-w......-w-w-E-C], [E-w-w-w-....w-w-w-C] or
        //                    [E-C-w-w-w-.......-w-w]? (first and third are the same only if we have fully used CPUs)
        // Now EWC is always applied
        if(emitterMappingRequired){
            _emitterIndex = 0;
            _firstWorkerIndex = 1;
            frequencyScalableVirtualCores.push_back(_availableVirtualCores.at(_emitterIndex));
        }

        if(collectorMappingRequired){
            _collectorIndex = (_firstWorkerIndex + _workers.size()) % _availableVirtualCores.size();
            frequencyScalableVirtualCores.push_back(_availableVirtualCores.at(_collectorIndex));
        }

        /** Perform mapping. **/
        AdaptiveNode* node = NULL;
        //TODO: Che succede se la farm ha l'emitter di default? :(
        if((node = _farm->getAdaptiveEmitter())){
            node->getThreadHandler()->move(_availableVirtualCores.at(_emitterIndex));
        }

        if((node = _farm->getAdaptiveCollector())){
            node->getThreadHandler()->move(_availableVirtualCores.at(_collectorIndex));
        }

        for(size_t i = 0; i < _workers.size(); i++){
            topology::VirtualCore* vc = _availableVirtualCores.at(((_firstWorkerIndex + i) %
                                                                   _availableVirtualCores.size()));
            frequencyScalableVirtualCores.push_back(vc);
            node = _workers.at(i);
            node->getThreadHandler()->move(vc);
        }
    }

    if(_adaptivityParameters->strategyFrequencies != STRATEGY_FREQUENCY_NO){
        /**
         *  We only change and set frequency to domains that contain at
         *  least one virtual core among those where workers are mapped.
         **/
        std::vector<cpufreq::Domain*> usedDomains = _adaptivityParameters->cpufreq->getDomains(frequencyScalableVirtualCores);

        for(size_t i = 0; i < usedDomains.size(); i++){
            std::vector<cpufreq::Frequency> availableFrequencies = usedDomains.at(i)->getAvailableFrequencies();
            usedDomains.at(i)->setGovernor(_adaptivityParameters->frequencyGovernor);

            if(_adaptivityParameters->frequencyGovernor != cpufreq::GOVERNOR_USERSPACE){
                usedDomains.at(i)->setGovernorBounds(_adaptivityParameters->frequencyLowerBound,
                                                     _adaptivityParameters->frequencyUpperBound);
            }else if(_adaptivityParameters->strategyFrequencies != STRATEGY_FREQUENCY_OS){
                usedDomains.at(i)->setHighestFrequencyUserspace();
            }
        }
    }
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

    mapAndSetFrequencies();

    std::cout << "Emitter node: " << _availableVirtualCores.at(_emitterIndex)->getVirtualCoreId() << std::endl;
    std::cout << "Collector node: " << _availableVirtualCores.at(_collectorIndex)->getVirtualCoreId() << std::endl;
    std::cout << "Worker nodes: ";
    for(size_t i = 0; i < adaptiveWorkers.size(); i++){
        std::cout <<  _availableVirtualCores.at((_firstWorkerIndex + i) % _availableVirtualCores.size())->getVirtualCoreId() << ", ";
    }
    std::cout << std::endl;

    _lock.lock();
    while(!_stop){
        _lock.unlock();
        std::cout << "Manager running" << std::endl;
        sleep(_adaptivityParameters->samplingInterval);
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
