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
 *
 * To let an existing fastflow farm-based adaptive, follow these steps:
 *  1. Emitter, Workers and Collector of the farm must extend mammut::fasflow::AdaptiveNode
 *     instead of ff::ff_node
 *  2. Replace (if present) svc_init with adaptive_svc_init
 *  3. Substitute ff::ff_farm with mammut::fastflow::AdaptiveFarm. The maximum number of workers
 *     that can be activated correspond to the number of workers specified during farm creation.
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

#include <iostream>

namespace mammut{
namespace fastflow{

using namespace ff;

class AdaptivityParameters;
class AdaptiveNode;

template<typename lb_t, typename gt_t>
class AdaptivityManagerFarm;

/// Possible reconfiguration strategies.
typedef enum{
    STRATEGY_FREQUENCY_NO = 0, ///< Does not reconfigure frequencies.
    STRATEGY_FREQUENCY_OS, ///< Reconfigures the number of workers. The frequencies are managed by OS governor.
                           ///< The governor must be specified with 'setFrequengyGovernor' call.
    STRATEGY_FREQUENCY_CORES_CONSERVATIVE, ///< Reconfigures frequencies and number of workers. Tries always to
                                           ///< minimize the number of virtual cores used.
    STRATEGY_FREQUENCY_POWER_CONSERVATIVE ///< Reconfigures frequencies and number of workers. Tries to minimize
                                          ///< the consumed power.
}StrategyFrequencies;

/// Possible mapping strategies.
typedef enum{
    STRATEGY_MAPPING_OS = 0, ///< Mapping decisions will be performed by the operating system.
    STRATEGY_MAPPING_AUTO, ///< Mapping strategy will be automatically chosen at runtime.
    STRATEGY_MAPPING_LINEAR, ///< Tries to keep the threads as close as possible.
    STRATEGY_MAPPING_CACHE_EFFICIENT ///< Tries to make good use of the shared caches.
                                     ///< Particularly useful when threads have large working sets.
}StrategyMapping;

/// Possible parameters validation results.
typedef enum{
    VALIDATION_OK = 0, ///< Parameters are ok.
    VALIDATION_STRATEGY_FREQUENCY_UNSUPPORTED, ///< The specified frequency strategy is not supported
                                               ///< on this machine.
    VALIDATION_GOVERNOR_UNSUPPORTED, ///< Specified governor not supported on this machine.
    VALIDATION_STRATEGY_MAPPING_UNSUPPORTED, ///< Specified mapping strategy not supported on this machine.
    VALIDATION_THRESHOLDS_INVALID, ///< Wrong value for overload and/or underload thresholds.
    VALIDATION_EC_SENSITIVE_WRONG_F_STRATEGY ///< sensitiveEmitter or sensitiveCollector specified but frequency
                                             ///< strategy is STRATEGY_FREQUENCY_NO.
}AdaptivityParametersValidation;

/*!
 * \class AdaptivityParameters
 * \brief This class contains parameters for adaptivity choices.
 *
 * This class contains parameters for adaptivity choices.
 */
class AdaptivityParameters{
private:
    template<typename lb_t, typename gt_t>
    friend class AdaptiveFarm;

    template<typename lb_t, typename gt_t>
    friend class AdaptivityManagerFarm;

    Communicator* const communicator;
    cpufreq::CpuFreq* cpufreq;
    energy::Energy* energy;
    topology::Topology* topology;

    bool isFrequencyGovernorAvailable(cpufreq::Governor governor);
public:
    StrategyFrequencies strategyFrequencies; ///< The frequency strategy.
    cpufreq::Governor frequencyGovernor; ///< The frequency governor (only used when
                                         ///< strategyFrequencies is STRATEGY_FREQUENCY_OS)
    StrategyMapping strategyMapping; ///< The mapping strategy.
    bool sensitiveEmitter; ///< If true, we will try to run the emitter at the highest possible
                           ///< frequency (only available when strategyFrequencies != STRATEGY_FREQUENCY_NO.
                           ///< In some cases it may still not be possible).
    bool sensitiveCollector; ///< If true, we will try to run the collector at the highest possible frequency
                             ///< (only available when strategyFrequencies != STRATEGY_FREQUENCY_NO.
                             ///< In some cases it may still not be possible).
    uint32_t numSamples; ///< The number of samples used to take reconfiguration decisions.
    uint32_t samplingInterval; ///<  The length of the sampling interval (in seconds) over which
                               ///< the reconfiguration decisions are taken.
    double underloadThresholdFarm; ///< The underload threshold for the entire farm.
    double overloadThresholdFarm; ///< The overload threshold for the entire farm.
    double underloadThresholdWorker; ///< The underload threshold for a single worker.
    double overloadThresholdWorker; ///< The overload threshold for a single worker.
    bool migrateCollector; ///< If true, when a reconfiguration occur, the collector is migrated to a
                           ///< different virtual core (if needed).
    uint32_t stabilizationPeriod; ///< The minimum number of seconds that must elapse between two successive
                                  ///< reconfiguration.

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

/*!private
 * \class AdaptiveNode
 * \brief This class wraps a ff_node to let it reconfigurable.
 *
 * This class wraps a ff_node to let it reconfigurable.
 */
class AdaptiveNode: public ff_node{
private:
    template<typename lb_t, typename gt_t>
    friend class AdaptiveFarm;

    template<typename lb_t, typename gt_t>
    friend class AdaptivityManagerFarm;

    task::TasksManager* _tasksManager;
    task::ThreadHandler* _thread;
    bool _firstInit;

    /**
     * Returns the thread handler associated to this node.
     * If it is called before running the node, an exception
     * is thrown.
     * @return The thread handler associated to this node.
     *         It doesn't need to be released.
     */
    task::ThreadHandler* getThreadHandler() const;

    /**
     * Initializes the mammut modules needed by the node.
     * @param communicator A communicator. If NULL, the modules
     *        will be initialized locally.
     */
    void initMammutModules(Communicator* const communicator);
public:
    /**
     * Builds an adaptive node.
     */
    AdaptiveNode();

    /**
     * Destroyes this adaptive node.
     */
    ~AdaptiveNode();

    /**
     * \internal
     * Wraps the user svc_init with adaptivity logics.
     * @return 0 for success, != 0 otherwise.
     */
    int svc_init() CX11_KEYWORD(final);

    /**
     * The class that extends AdaptiveNode, must replace
     * (if present) the declaration of svc_init with
     * adaptive_svc_init.
     * @return 0 for success, != 0 otherwise.
     */
    virtual int adaptive_svc_init();
};

/*!
 * \class AdaptiveFarm
 * \brief This class wraps a farm to let it reconfigurable.
 *
 * This class wraps a farm to let it reconfigurable.
 */
template<typename lb_t=ff_loadbalancer, typename gt_t=ff_gatherer>
class AdaptiveFarm: public ff_farm<lb_t, gt_t>{
    friend class AdaptivityManagerFarm<lb_t, gt_t>;
private:
    bool _firstRun;
    AdaptivityParameters* _adaptivityParameters;
    AdaptivityManagerFarm<lb_t, gt_t>* _adaptivityManager;
    void construct(AdaptivityParameters* adaptivityParameters);
public:
    /**
     * Builds the adaptive farm.
     * For parameters documentation, see fastflow's farm documentation.
     * @param adaptivityParameters Parameters that will be used by the farm to take reconfiguration decisions.
     */
    AdaptiveFarm(AdaptivityParameters* adaptivityParameters, std::vector<ff_node*>& w,
                 ff_node* const emitter = NULL, ff_node* const collector = NULL, bool inputCh = false);

    /**
     * Builds the adaptive farm.
     * For parameters documentation, see fastflow's farm documentation.
     * @param adaptivityParameters Parameters that will be used by the farm to take reconfiguration decisions.
     */
    explicit AdaptiveFarm(AdaptivityParameters* adaptivityParameters = NULL, bool inputCh = false,
                          int inBufferEntries = ff_farm<lb_t, gt_t>::DEF_IN_BUFF_ENTRIES,
                          int outBufferEntries = ff_farm<lb_t, gt_t>::DEF_OUT_BUFF_ENTRIES,
                          bool workerCleanup = false,
                          int maxNumWorkers = ff_farm<lb_t, gt_t>::DEF_MAX_NUM_WORKERS,
                          bool fixedSize = false);

    /**
     * Destroyes this adaptive farm.
     */
    ~AdaptiveFarm();

    /**
     * Runs this farm.
     */
    int run(bool skip_init=false);

    /**
     * Waits this farm for completion.
     */
    int wait();
};

/*!
 * \internal
 * Represents possible linear mappings of [Emitter, Workers, Collector]
 */
typedef enum{
    LINEAR_MAPPING_EWC = 0, ///< [Emitter, Workers, Collector]
    LINEAR_MAPPING_WEC, ///< [Workers, Emitter, Collector]
    LINEAR_MAPPING_ECW ///< [Emitter, Collector, Workers]
}LinearMappingType;

/*!
 * \internal
 * \class AdaptivityManagerFarm
 * \brief This class manages the adaptivity in farm based computations.
 *
 * This class manages the adaptivity in farm based computations.
 */
template<typename lb_t=ff_loadbalancer, typename gt_t=ff_gatherer>
class AdaptivityManagerFarm: public utils::Thread{
private:
    AdaptiveFarm<lb_t, gt_t>* _farm;
    AdaptivityParameters* _adaptivityParameters;
    svector<ff_node*> _workers;
    bool _stop;
    utils::LockPthreadMutex _lock;
    std::vector<topology::VirtualCore*> _availableVirtualCores;
    size_t _firstWorkerIndex; ///< The index of the first worker in _availableVirtualCores vector.
                              ///< Accordingly, _availableVirtualCores[_firstWorkerIndex,
                              ///<                                     _firstWorkerIndex + numWorkers - 1]
                              ///< will be the virtual cores where the workers of the farm are mapped.
    size_t _emitterIndex; ///< The index of the emitter in _availableVirtualCores vector.
    size_t _collectorIndex; ///< The index of the collector in _availableVirtualCores vector.

    /**
     * If possible, finds a set of physical cores that can always run at the highest frequency.
     * These physical cores will be chosen among those not used by the workers.
     * @param workersVirtualCores The virtual cores that will be used by the workers.
     * @return A set of physical cores that can always run at the highest frequency.
     */
    std::vector<topology::PhysicalCore*> findPossiblePerformancePhysicalCores(const std::vector<topology::VirtualCore*>& workersVirtualCores) const;

    /**
     * Set a specified virtual core to the highest frequency.
     * @param virtualCore The virtual core.
     * @param nodeIndex The index of the virtual core in _availableVirtualCores vector.
     * @return true if the virtual core has been set to the highest frequency, false otherwise.
     */
    bool setVirtualCoreToHighestFrequency(topology::VirtualCore* virtualCore, size_t& nodeIndex);

public:
    /**
     * Creates a farm adaptivity manager.
     * @param farm The farm to be managed.
     * @param adaptivityParameters The parameters to be used for adaptivity decisions.
     */
    AdaptivityManagerFarm(AdaptiveFarm<lb_t, gt_t>* farm, AdaptivityParameters* adaptivityParameters);

    /**
     * Destroyes this adaptivity manager.
     */
    ~AdaptivityManagerFarm();

    /**
     * Prepares the frequencies and governors for running.
     */
    void initCpuFreq();

    /**
     * Prepares the mapping of the threads on virtual cores.
     */
    void setMapping();

    /**
     * Function executed by this thread.
     */
    void run();

    /**
     * Stops this manager.
     */
    void stop();
};

}
}

#include "fastflow.tpp"


#endif /* MAMMUT_FASTFLOW_HPP_ */
