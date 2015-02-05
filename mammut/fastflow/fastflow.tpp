#ifndef MAMMUT_FASTFLOW_TPP_
#define MAMMUT_FASTFLOW_TPP_

namespace mammut{
namespace fastflow{

template <typename lb_t, typename gt_t>
void AdaptiveFarm<lb_t, gt_t>::construct(AdaptivityParameters* adaptivityParameters){
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
    svector<ff_node*> workers = ff_farm<lb_t, gt_t>::getWorkers();
    for(size_t i = 0; i < workers.size(); i++){
        (static_cast<AdaptiveWorker*>(workers[i]))->initMammutModules(_adaptivityParameters->communicator);
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
    _workers(_farm->getWorkers()),
    _stop(false){
    ;
}

template<typename lb_t, typename gt_t>
AdaptivityManagerFarm<lb_t, gt_t>::~AdaptivityManagerFarm(){
    ;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::initCpuFreq(){
    std::vector<cpufreq::Frequency> availableFrequencies;
    std::vector<topology::VirtualCore*> map;
    topology::VirtualCore* emitterVirtualCore = NULL;
    topology::VirtualCore* collectorVirtualCore = NULL;

    //TODO: Better to map [E-w-w-w-....w-w-w-C], [E-C-w-w-w-.......-w-w] or [w-w-w-w-w......-w-w-E-C]? (second and third are the same only if we have fully used CPUs)
    switch(_adaptivityParameters->strategyMapping){
        case STRATEGY_MAPPING_OS:{
            return;
        }break;
        case STRATEGY_MAPPING_AUTO: //TODO: Auto should choose between all the others
        case STRATEGY_MAPPING_LINEAR:{
           /*
            * Generates a vector of virtual cores to be used for linear mapping.
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
                        map.push_back(physicalCores.at(j)->getVirtualCores().at(virtualUsed));
                    }
                }
                ++virtualUsed;
            }
        }break;
        case STRATEGY_MAPPING_CACHE_EFFICIENT:{
            throw std::runtime_error("Not yet supported.");
        }
    }

    /**
     * If requested, and if there are available domains, run emitter or collector (or both) at
     * the highest frequency.
     */
    if(_adaptivityParameters->strategyFrequencies != STRATEGY_FREQUENCY_NO){
        if(_adaptivityParameters->sensitiveEmitter || _adaptivityParameters->sensitiveCollector){
            std::vector<cpufreq::Domain*> allDomains = _adaptivityParameters->cpufreq->getDomains();
            std::vector<topology::VirtualCore*> hypotheticWorkersMap(map.begin(), (_workers.size() < map.size())? map.begin() + _workers.size():map.end());
            std::vector<cpufreq::Domain*> hypotheticWorkersDomains = _adaptivityParameters->cpufreq->getDomains(hypotheticWorkersMap);
            std::vector<cpufreq::Domain*> unusedDomains;
            std::vector<topology::PhysicalCore*> physicalCoresInUnusedDomains;
            /** Find not used domains. **/
            if(allDomains.size() > hypotheticWorkersDomains.size()){
                for(size_t i = 0; i < allDomains.size(); i++){
                    cpufreq::Domain* currentDomain = allDomains.at(i);
                    if(!utils::contains(hypotheticWorkersDomains, currentDomain)){
                        unusedDomains.push_back(currentDomain);
                        utils::append(physicalCoresInUnusedDomains,
                                      _adaptivityParameters->topology->virtualToPhysical(currentDomain->getVirtualCores()));
                    }
                }

                //TODO Refactor the following two pieces
                if(physicalCoresInUnusedDomains.size() && _adaptivityParameters->sensitiveEmitter){
                    emitterVirtualCore = physicalCoresInUnusedDomains.back();
                    physicalCoresInUnusedDomains.pop_back();
                    for(size_t i = 0; i < unusedDomains.size(); i++){
                        cpufreq::Domain* currentUnusedDomain = unusedDomains.at(i);
                        if(currentUnusedDomain->contains(emitterVirtualCore)){
                            if(!currentUnusedDomain->setGovernor(mammut::cpufreq::GOVERNOR_PERFORMANCE)){
                                currentUnusedDomain->setHighestFrequencyUserspace();
                            }
                            break;
                        }
                    }
                }

                if(physicalCoresInUnusedDomains.size() && _adaptivityParameters->sensitiveCollector){
                    collectorVirtualCore = physicalCoresInUnusedDomains.back();
                    physicalCoresInUnusedDomains.pop_back();
                    for(size_t i = 0; i < unusedDomains.size(); i++){
                        cpufreq::Domain* currentUnusedDomain = unusedDomains.at(i);
                        if(currentUnusedDomain->contains(emitterVirtualCore)){
                            if(!currentUnusedDomain->setGovernor(mammut::cpufreq::GOVERNOR_PERFORMANCE)){
                                currentUnusedDomain->setHighestFrequencyUserspace();
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    /** Performs mapping. **/
    std::vector<topology::VirtualCore*> workersMap;
    if(map.size()){
        for(size_t i = 0; i < _workers.size(); i++){
            workersMap.push_back(map.at(i % map.size()));
            static_cast<AdaptiveWorker*>(_workers[i])->move(workersMap.at(i)->getVirtualCoreId());
        }
    }

    if(_adaptivityParameters->strategyFrequencies != STRATEGY_FREQUENCY_NO){
        /**
         *  We only change and set frequency to domains that contain at
         *  least one virtual core among those where workers are mapped.
         **/
        std::vector<cpufreq::Domain*> usedDomains = _adaptivityParameters->cpufreq->getDomains(workersMap);

        for(size_t i = 0; i < usedDomains.size(); i++){
            availableFrequencies = usedDomains.at(i)->getAvailableFrequencies();
            usedDomains.at(i)->setGovernor(_adaptivityParameters->frequencyGovernor);

            if(_adaptivityParameters->frequencyGovernor != cpufreq::GOVERNOR_USERSPACE){
                //TODO: Add a call to specify bounds in os governor based strategies
                usedDomains.at(i)->setGovernorBounds(availableFrequencies.at(0),
                                                             availableFrequencies.at(availableFrequencies.size() - 1));
            }else if(_adaptivityParameters->strategyFrequencies != STRATEGY_FREQUENCY_OS){
                usedDomains.at(i)->setFrequencyUserspace(availableFrequencies.at(availableFrequencies.size() - 1));
            }
        }
    }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::setMapping(){
    ;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::run(){
    initCpuFreq();
    setMapping();

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
