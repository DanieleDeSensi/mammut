#ifndef MAMMUT_FASTFLOW_TPP_
#define MAMMUT_FASTFLOW_TPP_

template <typename W, typename lb_t, typename gt_t>
AdaptiveFarm<W, lb_t, gt_t>::AdaptiveFarm(AdaptivityParameters &adaptivityParameters): //TODO: Implement ff_farm constructors.
    _adaptivityParameters(adaptivityParameters){
    ;
}

template <typename W, typename lb_t, typename gt_t>
AdaptiveFarm<W, lb_t, gt_t>::~AdaptiveFarm(){
    for(size_t i = 0; i < _realWorkers.size(); i++){
        delete static_cast<AdaptiveWorker<W>* >(_realWorkers.at(i));
    }
    _realWorkers.clear();
}

template <typename W, typename lb_t, typename gt_t>
int AdaptiveFarm<W, lb_t, gt_t>::add_workers(std::vector<ff::ff_node *> & w){
    for(size_t i = 0; i < w.size(); i++){
        _realWorkers.push_back(new AdaptiveWorker<W>(dynamic_cast<W*>(w.at(i)), _adaptivityParameters.communicator));
    }
    return ff::ff_farm<lb_t, gt_t>::add_workers(_realWorkers);
}

template<typename W, typename lb_t, typename gt_t>
FarmAdaptivityManager<W, lb_t, gt_t>::FarmAdaptivityManager(AdaptiveFarm<W, lb_t, gt_t>* farm, AdaptivityParameters& adaptivityParameters):
    _farm(farm),
    _adaptivityParameters(adaptivityParameters){
    uint validationRes = _adaptivityParameters.validate();
    if(validationRes != VALIDATION_OK){
        throw std::runtime_error("FarmAdaptivityManager: invalid AdaptivityParameters: " + utils::intToString(validationRes));
    }
}

template<typename W, typename lb_t, typename gt_t>
FarmAdaptivityManager<W, lb_t, gt_t>::~FarmAdaptivityManager(){
    ;
}

template<typename W, typename lb_t, typename gt_t>
void FarmAdaptivityManager<W, lb_t, gt_t>::initCpuFreq(){
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

template<typename W, typename lb_t, typename gt_t>
void FarmAdaptivityManager<W, lb_t, gt_t>::setMapping(){
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

template<typename W, typename lb_t, typename gt_t>
void FarmAdaptivityManager<W, lb_t, gt_t>::run(){
    if(_farm){
        throw std::runtime_error("FarmAdaptivityManager: setFarm() must be called before run().");
    }

    initCpuFreq();
    setMapping();

    while(true){
        sleep(_adaptivityParameters.samplingInterval);
        _farm->get_workers();
    }
}

#endif
