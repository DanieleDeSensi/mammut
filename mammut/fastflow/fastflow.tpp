#ifndef MAMMUT_FASTFLOW_TPP_
#define MAMMUT_FASTFLOW_TPP_

template <typename lb_t, typename gt_t>
void AdaptiveFarm<lb_t, gt_t>::construct(AdaptivityParameters adaptivityParameters){
    _firstRun = true;
    _adaptivityParameters = adaptivityParameters;
    _adaptivityManager = NULL;
}

template <typename lb_t, typename gt_t>
AdaptiveFarm<lb_t, gt_t>::AdaptiveFarm(AdaptivityParameters adaptivityParameters, std::vector<ff_node*>& w, 
                                       ff_node* const emitter, ff_node* const collector, bool inputCh):
    ff_farm<lb_t, gt_t>::ff_farm(w, emitter, collector, inputCh){
    construct(adaptivityParameters);
}

template <typename lb_t, typename gt_t>
AdaptiveFarm<lb_t, gt_t>::AdaptiveFarm(AdaptivityParameters adaptivityParameters, bool inputCh,
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
    if(_adaptivityManager){
        delete _adaptivityManager;
    }
}

template <typename lb_t, typename gt_t>
int AdaptiveFarm<lb_t, gt_t>::run(bool skip_init){
    if(_firstRun){
        _firstRun = false;
        _adaptivityManager = new AdaptivityManagerFarm<lb_t, gt_t>(this, _adaptivityParameters);
    }
    std::cout << "Run adaptive farm." << std::endl;
    return ff_farm<lb_t, gt_t>::run(skip_init);
}

template<typename lb_t, typename gt_t>
AdaptivityManagerFarm<lb_t, gt_t>::AdaptivityManagerFarm(AdaptiveFarm<lb_t, gt_t>* farm, AdaptivityParameters adaptivityParameters):
    _farm(farm),
    _adaptivityParameters(adaptivityParameters){
    uint validationRes = _adaptivityParameters.validate();
    if(validationRes != VALIDATION_OK){
        throw std::runtime_error("AdaptivityManagerFarm: invalid AdaptivityParameters: " + utils::intToString(validationRes));
    }
    _workers = _farm->getWorkers();
    for(size_t i = 0; i < _workers.size(); i++){
        (static_cast<AdaptiveWorker*>(_workers[i]))->initMammutModules(_adaptivityParameters.communicator);
    }
}

template<typename lb_t, typename gt_t>
AdaptivityManagerFarm<lb_t, gt_t>::~AdaptivityManagerFarm(){
    ;
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::initCpuFreq(){
    if(_adaptivityParameters.strategyFrequencies != STRATEGY_FREQUENCY_NO){
         std::vector<cpufreq::Domain*> frequencyDomains = frequencyDomains = _adaptivityParameters.cpufreq->getDomains();
         std::vector<cpufreq::Frequency> availableFrequencies;
         for(size_t i = 0; i < frequencyDomains.size(); i++){
             availableFrequencies = frequencyDomains.at(i)->getAvailableFrequencies();
             frequencyDomains.at(i)->changeGovernor(_adaptivityParameters.frequencyGovernor);

             if(_adaptivityParameters.frequencyGovernor != cpufreq::GOVERNOR_USERSPACE){
                 //TODO: Add a call to specify bounds in os governor based strategies
                 frequencyDomains.at(i)->changeGovernorBounds(availableFrequencies.at(0),
                                                              availableFrequencies.at(availableFrequencies.size() - 1));
             }else if(_adaptivityParameters.strategyFrequencies != STRATEGY_FREQUENCY_OS){
                 //TODO: If possible, run emitter and collector at higher frequencies.
                 frequencyDomains.at(i)->changeFrequency(availableFrequencies.at(availableFrequencies.size() - 1));
             }
         }
     }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::setMapping(){
    switch(_adaptivityParameters.strategyMapping){
        case STRATEGY_MAPPING_OS:{
            return;
        }break;
        case STRATEGY_MAPPING_AUTO: //TODO: Auto should choose between all the others
        case STRATEGY_MAPPING_LINEAR:{
            std::vector<topology::VirtualCore*> virtualCores = _adaptivityParameters.topology->getVirtualCores();
        }break;
        case STRATEGY_MAPPING_CACHE_EFFICIENT:{
            throw std::runtime_error("Not yet supported.");
        }
    }
}

template<typename lb_t, typename gt_t>
void AdaptivityManagerFarm<lb_t, gt_t>::run(){
    if(_farm){
        throw std::runtime_error("AdaptivityManagerFarm: setFarm() must be called before run().");
    }

    initCpuFreq();
    setMapping();

    while(true){
        std::cout << "Manager running" << std::endl;
        sleep(_adaptivityParameters.samplingInterval);
    }
}

#endif
