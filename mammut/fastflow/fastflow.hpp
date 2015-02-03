/*
 * fastflow.hpp
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

/*! \file fastflow.hpp
 * \brief Contains utilities for integration with FastFlow programs.
 *
 * Contains utilities for integration with FastFlow programs.
 * FastFlow (http://calvados.di.unipi.it/dokuwiki/doku.php?id=ffnamespace:about) is
 * a parallel programming framework for multicore platforms based upon non-blocking
 * lock-free/fence-free synchronization mechanisms.
 */

#ifndef MAMMUT_FASTFLOW_HPP_
#define MAMMUT_FASTFLOW_HPP_

#include <ff/farm.hpp>
#include <mammut/module.hpp>
#include <mammut/utils.hpp>
#include <mammut/cpufreq/cpufreq.hpp>
#include <mammut/energy/energy.hpp>
#include <mammut/task/task.hpp>
#include <mammut/topology/topology.hpp>


namespace mammut{
namespace fastflow{

class AdaptivityParameters;

/*!
 * \internal
 * \class AdaptiveWorker
 * \brief This class wraps a farm worker to let it reconfigurable.
 *
 * This class wraps a farm worker to let it reconfigurable.
 */
template <class T>
class AdaptiveWorker: public T
{
private:
    T* _realWorker;
    task::TasksManager* _tasksManager;
    task::ThreadHandler* _thread;
    topology::VirtualCoreId _virtualCoreId;
    bool _firstInit;
public:
    AdaptiveWorker(T* realWorker, Communicator* const communicator):
        _realWorker(realWorker),
        _thread(NULL),
        _virtualCoreId(0),
        _firstInit(true){
        utils::assertDerivedFrom<T, ff::ff_node>();
        if(communicator){
            _tasksManager = task::TasksManager::remote(communicator);
        }else{
            _tasksManager = task::TasksManager::local();
        }
    }

    ~AdaptiveWorker(){
        if(_thread){
            _tasksManager->releaseThreadHandler(_thread);
        }

        if(_tasksManager){
            task::TasksManager::release(_tasksManager);
        }
    }

    /** Passes all the calls performed on a ReconfigurableWorker to the original wrapped worker. **/
    T operator ->(){
          return _realWorker;
    }

    void move(topology::VirtualCoreId virtualCoreId){
        _virtualCoreId = virtualCoreId;
        if(_thread){
            _thread->move(_virtualCoreId);
        }
    }

    int svc_init(){
        if(_firstInit){
            /** Operations performed only the first time the thread is running. **/
            _firstInit = false;
            _thread = _tasksManager->getThreadHandler();
            move(_virtualCoreId);
        }

        return _realWorker->svc_init();
    }

    void* svc(void* task){
        return _realWorker->svc(task);
    }
};

/*!
 * \class AdaptiveFarm
 * \brief This class wraps a farm to let it reconfigurable.
 *
 * This class wraps a farm to let it reconfigurable.
 */
template<typename W, typename lb_t=ff::ff_loadbalancer, typename gt_t=ff::ff_gatherer>
class AdaptiveFarm: public ff::ff_farm<lb_t, gt_t>{
private:
    AdaptivityParameters& _adaptivityParameters;
    std::vector<ff::ff_node*> _realWorkers;
public:
    AdaptiveFarm(AdaptivityParameters& adaptivityParameters);

    ~AdaptiveFarm();

    int add_workers(std::vector<ff::ff_node *> & w);
};


/*!
 * \internal
 * \class FarmAdaptivityManager
 * \brief This class manages the adaptivity in farm based computations.
 *
 * This class manages the adaptivity in farm based computations.
 */
template<typename W, typename lb_t=ff::ff_loadbalancer, typename gt_t=ff::ff_gatherer>
class FarmAdaptivityManager: public utils::Thread{
public:
    AdaptivityParameters& _adaptivityParameters;
    AdaptiveFarm<W, lb_t, gt_t>* _farm;

    /**
     * Creates a farm adaptivity manager.
     * @param farm The farm to be managed.
     * @param adaptivityParameters The parameters to be used for adaptivity decisions.
     */
    FarmAdaptivityManager(AdaptiveFarm<W, lb_t, gt_t>* farm, AdaptivityParameters& adaptivityParameters);

    /**
     * Destroyes this adaptivity manager.
     */
    ~FarmAdaptivityManager();


    /**
     * Prepares the frequencies and governors for running.
     */
    void initCpuFreq();

    /**
     * Prepares the mapping of the threads on virtual cores.
     */
    void setMapping();

    void run();
};

/// Possible reconfiguration strategies.
typedef enum{
    STRATEGY_FREQUENCY_NO = 0, ///< Does not reconfigure frequencies.
    STRATEGY_FREQUENCY_OS, ///< Reconfigures the number of workers. The frequencies are managed by OS governor. The governor must be specified with 'setFrequengyGovernor' call.
    STRATEGY_FREQUENCY_CORES_CONSERVATIVE, ///< Reconfigures frequencies and number of workers. Tries always to minimize the number of virtual cores used.
    STRATEGY_FREQUENCY_POWER_CONSERVATIVE ///< Reconfigures frequencies and number of workers. Tries to minimize the consumed power.
}StrategyFrequencies;

/// Possible mapping strategies.
typedef enum{
    STRATEGY_MAPPING_OS = 0, ///< Mapping decisions will be performed by the operating system.
    STRATEGY_MAPPING_AUTO, ///< Mapping strategy will be automatically chosen at runtime.
    STRATEGY_MAPPING_LINEAR, ///< Tries to keep the threads as close as possible.
    STRATEGY_MAPPING_CACHE_EFFICIENT ///< Tries to make good use of the shared caches. Particularly useful when threads have large working sets.
}StrategyMapping;

/// Possible parameters validation results.
typedef enum{
    VALIDATION_OK = 0, ///< Parameters are ok.
    VALIDATION_STRATEGY_FREQUENCY_UNSUPPORTED, ///< The specified frequency strategy is not supported on this machine.
    VALIDATION_GOVERNOR_UNSUPPORTED, ///< Specified governor not supported on this machine.
    VALIDATION_STRATEGY_MAPPING_UNSUPPORTED, ///< Specified mapping strategy not supported on this machine.
    VALIDATION_THRESHOLDS_INVALID ///< Wrong value for overload and/or underload thresholds.
}AdaptivityParametersValidation;

/*!
 * \class AdaptivityParameters
 * \brief This class contains parameters for adaptivity choices.
 *
 * This class contains parameters for adaptivity choices.
 */
class AdaptivityParameters{
private:
    template<typename W, typename lb_t, typename gt_t>
    friend class AdaptiveFarm;

    template<typename W, typename lb_t, typename gt_t>
    friend class FarmAdaptivityManager;

    Communicator* const communicator;
    cpufreq::CpuFreq* cpufreq;
    energy::Energy* energy;
    topology::Topology* topology;

    bool isFrequencyGovernorAvailable(cpufreq::Governor governor);
public:
    StrategyFrequencies strategyFrequencies; ///< The frequency strategy.
    cpufreq::Governor frequencyGovernor; ///< The frequency governor (only used when strategyFrequencies is STRATEGY_FREQUENCY_OS)
    StrategyMapping strategyMapping; ///< The mapping strategy.
    uint32_t numSamples; ///< The number of samples used to take reconfiguration decisions.
    uint32_t samplingInterval; ///<  The length of the sampling interval (in seconds) over which the reconfiguration decisions are taken.
    double underloadThresholdFarm; ///< The underload threshold for the entire farm.
    double overloadThresholdFarm; ///< The overload threshold for the entire farm.
    double underloadThresholdWorker; ///< The underload threshold for a single worker.
    double overloadThresholdWorker; ///< The overload threshold for a single worker.
    bool migrateCollector; ///< If true, when a reconfiguration occur, the collector is migrated to a different virtual core (if needed).
    uint32_t stabilizationPeriod; ///< The minimum number of seconds that must elapse between two successive reconfiguration.

    /**
     * Creates the adaptivity parameters.
     * @param communicator The communicator used to instantiate the other modules.
     *        If NULL, the modules will be created as local modules.
     */
    AdaptivityParameters(Communicator* const communicator = NULL);

    /**
     * Destroyes these parameters.
     */
    ~AdaptivityParameters();

    /**
     * Validates these parameters.
     * @return The validation result.
     */
    AdaptivityParametersValidation validate();
};

#include "fastflow.tpp"

}
}

#endif /* MAMMUT_FASTFLOW_HPP_ */
