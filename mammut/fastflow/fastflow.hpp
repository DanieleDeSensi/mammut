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
 *  2. Replace the following calls (if present) in the farm nodes:
 *          svc           -> adp_svc
 *          svc_init      -> adp_svc_init
 *  3. If the application wants to be aware of the changes in the number of workers, the nodes
 *     can implement the notifyWorkersChange virtual method.
 *  4. Substitute ff::ff_farm with mammut::fastflow::AdaptiveFarm. The maximum number of workers
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
    STRATEGY_MAPPING_NO = 0, ///< Mapping decisions will be performed by the operating system.
    STRATEGY_MAPPING_AUTO, ///< Mapping strategy will be automatically chosen at runtime.
    STRATEGY_MAPPING_LINEAR, ///< Tries to keep the threads as close as possible.
    STRATEGY_MAPPING_CACHE_EFFICIENT ///< Tries to make good use of the shared caches.
                                     ///< Particularly useful when threads have large working sets.
}StrategyMapping;

/// Possible strategies to apply for unused virtual cores. For unused virtual cores we mean
/// those never used or those used only on some conditions.
typedef enum{
    STRATEGY_UNUSED_VC_NONE = 0, ///< Nothing is done on unused virtual cores.
    STRATEGY_UNUSED_VC_AUTO, ///< Automatically choose one of the other strategies.
    STRATEGY_UNUSED_VC_LOWEST_FREQUENCY, ///< Set the virtual cores to the lowest frequency (only
                                         ///< possible if all the other virtual cores on the same
                                         ///< domain are unused).
    STRATEGY_UNUSED_VC_OFF ///< Turn off the virtual cores. They will not be anymore seen by the
                           ///< operating system and it will not schedule anything on them.
}StrategyUnusedVirtualCores;

/// Possible parameters validation results.
typedef enum{
    VALIDATION_OK = 0, ///< Parameters are ok.
    VALIDATION_STRATEGY_FREQUENCY_REQUIRES_MAPPING, ///< strategyFrequencies can be different from STRATEGY_FREQUENCY_NO
                                                    ///< only if strategyMapping is different from STRATEGY_MAPPING_NO.
    VALIDATION_STRATEGY_FREQUENCY_UNSUPPORTED, ///< The specified frequency strategy is not supported
                                               ///< on this machine.
    VALIDATION_GOVERNOR_UNSUPPORTED, ///< Specified governor not supported on this machine.
    VALIDATION_STRATEGY_MAPPING_UNSUPPORTED, ///< Specified mapping strategy not supported on this machine.
    VALIDATION_THRESHOLDS_INVALID, ///< Wrong value for overload and/or underload thresholds.
    VALIDATION_EC_SENSITIVE_WRONG_F_STRATEGY, ///< sensitiveEmitter or sensitiveCollector specified but frequency
                                              ///< strategy is STRATEGY_FREQUENCY_NO.
    VALIDATION_EC_SENSITIVE_MISSING_GOVERNORS, ///< sensitiveEmitter or sensitiveCollector specified but highest
                                               ///< frequency can't be set..
    VALIDATION_INVALID_FREQUENCY_BOUNDS, ///< The bounds are invalid or the frequency strategy is not STRATEGY_FREQUENCY_OS.
    VALIDATION_UNUSED_VC_NO_OFF, ///< Strategy for unused virtual cores requires turning off the virtual cores but they
                                 ///< can't be turned off.
    VALIDATION_UNUSED_VC_NO_FREQUENCIES, ///< Strategy for unused virtual cores requires lowering the frequency but
                                         ///< frequency scaling not available.
    VALIDATION_WRONG_BANDWIDTH_PARAMETERS, ///< Specified bandwidth parameters are not valid.
    VALIDATION_VOLTAGE_FILE_NEEDED, ///< strategyFrequencies is STRATEGY_FREQUENCY_POWER_CONSERVATIVE but the voltage file
                                    ///< has not been specified or it does not exist.
    VALIDATION_NO_FAST_RECONF ///< Fast reconfiguration not available.
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
public:
    StrategyMapping strategyMapping; ///< The mapping strategy [default = STRATEGY_MAPPING_NO].
    StrategyFrequencies strategyFrequencies; ///< The frequency strategy. It can be different from
                                             ///< STRATEGY_FREQUENCY_NO only if strategyMapping is
                                             ///< different from STRATEGY_MAPPING_NO [default = STRATEGY_FREQUENCY_NO].
    cpufreq::Governor frequencyGovernor; ///< The frequency governor (only used when
                                         ///< strategyFrequencies is STRATEGY_FREQUENCY_OS) [default = GOVERNOR_USERSPACE].
    bool turboBoost; ///< Flag to enable/disable cores turbo boosting [default = false].
    cpufreq::Frequency frequencyLowerBound; ///< The frequency lower bound (only if strategyFrequency is
                                            ///< STRATEGY_FREQUENCY_OS) [default = unused].
    cpufreq::Frequency frequencyUpperBound; ///< The frequency upper bound (only if strategyFrequency is
                                            ///< STRATEGY_FREQUENCY_OS) [default = unused].
    bool fastReconfiguration; ///< If true, before changing the number of workers the frequency will be set to
                              ///< maximum to reduce the latency of the reconfiguration. The frequency will be
                              ///< be set again to the correct value after the farm is restarted [default = false].
    StrategyUnusedVirtualCores strategyUnusedVirtualCores; ///< Strategy for virtual cores that are never used
                                                           ///< [default = STRATEGY_UNUSED_VC_NONE].
    StrategyUnusedVirtualCores strategyInactiveVirtualCores; ///< Strategy for virtual cores that become inactive
                                                             ///< after a workers reconfiguration
                                                             ///< [default = STRATEGY_UNUSED_VC_NONE].
    bool sensitiveEmitter; ///< If true, we will try to run the emitter at the highest possible
                           ///< frequency (only available when strategyFrequencies != STRATEGY_FREQUENCY_NO.
                           ///< In some cases it may still not be possible) [default = false].
    bool sensitiveCollector; ///< If true, we will try to run the collector at the highest possible frequency
                             ///< (only available when strategyFrequencies != STRATEGY_FREQUENCY_NO.
                             ///< In some cases it may still not be possible) [default = false].
    uint32_t numSamples; ///< The number of samples used to take reconfiguration decisions [default = 10].
    uint32_t samplingInterval; ///<  The length of the sampling interval (in seconds) over which
                               ///< the reconfiguration decisions are taken [default = 1].
    double underloadThresholdFarm; ///< The underload threshold for the entire farm [default = 80.0].
    double overloadThresholdFarm; ///< The overload threshold for the entire farm [default = 90.0].
    double underloadThresholdWorker; ///< The underload threshold for a single worker [default = 80.0].
    double overloadThresholdWorker; ///< The overload threshold for a single worker [default = 90.0].
    bool migrateCollector; ///< If true, when a reconfiguration occur, the collector is migrated to a
                           ///< different virtual core (if needed) [default = false].
    uint32_t stabilizationSamples; ///< The minimum number of samples that must elapse between two successive
                                  ///< reconfiguration [default =  10].
    double requiredBandwidth; ///< The bandwidth required for the application (expressed as tasks/sec).
                              ///< If not specified, the application will adapt itself from time to time
                              ///< to the actual input bandwidth, respecting the conditions specified
                              ///< through underloadThresholdFarm and overloadThresholdFarm [default = unused].
    double maxBandwidthVariation; ///< The allowed variation for bandwidth. The bandwidth will be kept
                                  ///< Between [B - x, B + x] where B is 'requiredBandwidth' and x
                                  ///< is the 'maxBandwidthVariation' percentage of B [default = 5.0].
    std::string voltageTableFile; ///< The file containing the voltage table. It is mandatory when
                                  ///< strategyFrequencies is STRATEGY_FREQUENCY_POWER_CONSERVATIVE [default = unused].

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


/*!
 * \internal
 * \class NodeSample
 * \brief Represents a sample of values taken from an adaptive node.
 *
 * This struct represents a sample of values taken from an adaptive node.
 */
typedef struct NodeSample{
    double loadPercentage; ///< The percentage of time that the node spent on svc().
    double tasksCount; ///< The number of computed tasks.
    NodeSample():loadPercentage(0), tasksCount(0){;}
}NodeSample;

/*!
 * \internal
 * \class ManagementRequest
 * \brief Possible requests that a manager can make.
 */
typedef enum{
    MANAGEMENT_REQUEST_GET_AND_RESET_SAMPLE = 0, ///< Get the current sample and reset it.
    MANAGEMENT_REQUEST_PRODUCE_NULL ///< Produce a NULL value on output stream.
}ManagementRequest;

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
    utils::Monitor _threadCreated;
    uint64_t _tasksCount;
    ticks _workTicks;
    ticks _startTicks;
    ManagementRequest _managementRequest;
    NodeSample _sampleResponse;
    ff::SWSR_Ptr_Buffer _managementQ; ///< Queue used by the manager to notify that a request
                                      ///< is present on _managementRequest.
    ff::SWSR_Ptr_Buffer _responseQ; ///< Queue used by the node to notify that a response is
                                    ///< present on _sampleResponse.


    /**
     * Waits for the thread to be created.
     */
    void waitThreadCreation();

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

    /**
     * Returns the statistics computed since the last time this method has
     * been called.
     * @return The statistics computed since the last time this method has
     * been called.
     */
    NodeSample getAndResetSample();

    /**
     * Tell the node to produce a Null task as the next task.
     */
    void produceNull();
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
     * \internal
     * Wraps the user svc with adaptivity logics.
     * @param task The input task.
     * @return The output task.
     */
    void* svc(void* task) CX11_KEYWORD(final);

    /**
     * The class that extends AdaptiveNode, must replace
     * the declaration of svc with adp_svc.
     */
    virtual void* adp_svc(void* task) = 0;

    /**
     * The class that extends AdaptiveNode, must replace
     * (if present) the declaration of svc_init with
     * adp_svc_init.
     * @return 0 for success, != 0 otherwise.
     */
    virtual int adp_svc_init();

    /**
     * This method can be implemented by the nodes to be aware of a change in the number
     * of workers.
     * When the farm is stopped and before running it again with the new number of workers,
     * this method is called. It is called on the emitter (if present), on the collector (if
     * present) and on all the workers of the new configuration.
     * In this way, if needed action may be taken to prepare for the new configuration (e.g.
     * shared state modification, etc..).
     * @param oldNumWorkers The old number of workers.
     * @param newNumWorkers The new number of workers.
     */
    virtual void notifyWorkersChange(size_t oldNumWorkers, size_t newNumWorkers){;}
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
    std::vector<AdaptiveNode*> _adaptiveWorkers;
    AdaptiveNode* _adaptiveEmitter;
    AdaptiveNode* _adaptiveCollector;
    bool _firstRun;
    AdaptivityParameters* _adaptivityParameters;
    AdaptivityManagerFarm<lb_t, gt_t>* _adaptivityManager;

    void construct(AdaptivityParameters* adaptivityParameters);
    std::vector<AdaptiveNode*> getAdaptiveWorkers() const;
    AdaptiveNode* getAdaptiveEmitter() const;
    AdaptiveNode* getAdaptiveCollector() const;
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
typedef enum{ //TODO
    LINEAR_MAPPING_EWC = 0, ///< [Emitter, Workers, Collector]
    LINEAR_MAPPING_WEC, ///< [Workers, Emitter, Collector]
    LINEAR_MAPPING_ECW ///< [Emitter, Collector, Workers]
}LinearMappingType;

/*!
 * \internal
 * \struct FarmConfiguration
 * \brief Represents a possible farm configuration.
 *
 * This struct represent a possible farm configuration.
 */
typedef struct FarmConfiguration{
    uint numWorkers;
    cpufreq::Frequency frequency;
    FarmConfiguration():numWorkers(0), frequency(0){;}
    FarmConfiguration(uint numWorkers, cpufreq::Frequency frequency = 0):numWorkers(numWorkers), frequency(frequency){;}
}FarmConfiguration;

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
    bool _stop; ///< When true, the manager is stopped.
    utils::LockPthreadMutex _lock; ///< Used to let the manager stop safe.
    AdaptiveFarm<lb_t, gt_t>* _farm; ///< The managed farm.
    AdaptivityParameters* _p; ///< The parameters used to take management decisions.
    AdaptiveNode* _emitter; ///< The emitter (if present).
    AdaptiveNode* _collector; ///< The collector (if present).
    std::vector<AdaptiveNode*> _activeWorkers; ///< The currently running workers.
    std::vector<AdaptiveNode*> _inactiveWorkers; ///< Workers that can run but are not currently running.
    size_t _maxNumWorkers; ///< The maximum number of workers that can be activated by the manager.
    bool _emitterSensitivitySatisfied; ///< If true, the user requested sensitivity for emitter and the
                                       ///< request has been satisfied.
    bool _collectorSensitivitySatisfied; ///< If true, the user requested sensitivity for collector and the
                                         ///< request has been satisfied.
    FarmConfiguration _currentConfiguration; ///< The current configuration of the farm.
    const std::vector<topology::VirtualCore*> _availableVirtualCores; ///< The available virtual cores, sorted according to
                                                                      ///< the mapping strategy.
    std::vector<topology::VirtualCore*> _activeWorkersVirtualCores; ///< The virtual cores where the active workers
                                                                    ///< are running.
    std::vector<topology::VirtualCore*> _inactiveWorkersVirtualCores; ///< The virtual cores where the inactive workers
                                                                      ///< are running.
    std::vector<topology::VirtualCore*> _unusedVirtualCores; ///< Virtual cores not used by the farm nodes.
    topology::VirtualCore* _emitterVirtualCore; ///< The virtual core where the emitter (if present) is running.
    topology::VirtualCore* _collectorVirtualCore; ///< The virtual core where the collector (if present) is running.
    std::vector<cpufreq::Domain*> _scalableDomains; ///< The domains on which frequency scaling is applied.
    cpufreq::VoltageTable _voltageTable; ///< The voltage table.
    std::vector<cpufreq::Frequency> _availableFrequencies; ///< The available frequencies on this machine.
    std::vector<std::vector<NodeSample> > _nodeSamples; ///< The samples taken from the active workers.
    size_t _numRegisteredSamples; ///< The number of registered samples up to now.

    /**
     * If possible, finds a set of physical cores belonging to domains different from
     * those of virtual cores in 'virtualCores' vector.
     * @param virtualCores A vector of virtual cores.
     * @return A set of physical cores that can always run at the highest frequency.
     */
    std::vector<topology::PhysicalCore*> getSeparatedDomainPhysicalCores(const std::vector<topology::VirtualCore*>& virtualCores) const;

    /**
     * Set a specified domain to the highest frequency.
     * @param domain The domain.
     */
    void setDomainToHighestFrequency(const cpufreq::Domain* domain);

    /**
     * Computes the available virtual cores, sorting them according to the specified
     * mapping strategy.
     * @return The available virtual cores, sorted according to the specified mapping
     *         strategy.
     */
    std::vector<topology::VirtualCore*> getAvailableVirtualCores();

    /**
     * If requested, and if there are available domains, arrange nodes such that
     * emitter or collector (or both) can be run at the highest frequency.
     */
    void manageSensitiveNodes();

    /**
     * Generates mapping indexes. They are indexes to be used on _availableVirtualCores vector
     * to get the corresponding virtual core where a specific node must be mapped.
     * @param emitterIndex The index of the emitter.
     * @param firstWorkerIndex The index of the first worker (the others follow).
     * @param collectorIndex The index of the collector (if present).
     */
    void getMappingIndexes(size_t& emitterIndex, size_t& firstWorkerIndex, size_t& collectorIndex);

    /**
     * Computes the virtual cores where the nodes must be mapped
     * and pins these nodes on the virtual cores.
     */
    void mapNodesToVirtualCores();

    /**
     * Apply a specific strategy for a specified set of virtual cores.
     * @param strategyUnused The unused strategy.
     * @param unusedVirtualCores The virtual cores.
     */
    void applyUnusedVirtualCoresStrategy(StrategyUnusedVirtualCores strategyUnused, const std::vector<topology::VirtualCore*>& unusedVirtualCores);

    /**
     * Apply the strategies for inactive and unused virtual cores.
     */
    void applyUnusedVirtualCoresStrategy();

    /**
     * Updates the scalable domains vector.
     */
    void updateScalableDomains();

    /**
     * Set a specific P-state for the virtual cores used by
     * the current active workers, emitter (if not sensitive) and
     * collector (if not sensitive).
     * @param frequency The frequency to be set.
     */
    void updatePstate(cpufreq::Frequency frequency);

    /**
     * Map the nodes to virtual cores and
     * prepares frequencies and governors for running.
     */
    void mapAndSetFrequencies();

    /**
     * Returns the average load of the worker in position
     * workerId in the vector _workers.
     * @param workerId The index of the worker in _workers vector.
     * @return The average load of the worker in position
     * workerId in the vector _workers.
     */
    double getWorkerAverageLoad(size_t workerId);

    /**
     * Returns the average load of the farm.
     * @return The average load of the farm.
     */
    double getFarmAverageLoad();

    /**
     * Returns the average bandwidth (tasks/sec) of the worker in position
     * workerId in the vector _workers.
     * @param workerId The index of the worker in _workers vector.
     * @return The average bandwidth (tasks/sec) of the worker in position
     * workerId in the vector _workers.
     */
    double getWorkerAverageBandwidth(size_t workerId);

    /**
     * Returns the average bandwidth (tasks/sec) of the farm.
     * @return The average bandwidth (tasks/sec) of the farm.
     */
    double getFarmAverageBandwidth();

    /**
     * It returns the monitored value.
     * Its meaning depends on the parameters specified by the user.
     * It could be the current bandwidth, the current load, etc...
     * @return The monitored value.
     */
    double getMonitoredValue();

    /**
     * Checks if the contract requested by the user has been violated.
     * @return true if the contract has been violated, false otherwise.
     */
    bool isContractViolated(double monitoredValue) const;

    /**
     * Returns the estimated monitored value at a specific configuration.
     * @param monitoredValue The current monitored value.
     * @param configuration A possible future configuration.
     * @return The estimated monitored value at a specific configuration.
     */
    double getEstimatedMonitoredValue(double monitoredValue, const FarmConfiguration& configuration) const;

    /**
     * Returns the estimated power at a specific configuration.
     * @param configuration A possible future configuration.
     * @return The estimated power at a specific configuration.
     */
    double getEstimatedPower(const FarmConfiguration& configuration) const;

    /**
     * Returns a value that can never be assumed by the monitored value.
     * @return A value that can never be assumed by the monitored value.
     */
    double getImpossibleMonitoredValue() const;

    /**
     * Checks if x is a best suboptimal monitored value than y.
     * @param x The first monitored value.
     * @param y The second monitored value.
     * @return True if x is a best suboptimal monitored value than y, false otherwise.
     */
    bool isBestSuboptimalValue(double x, double y) const;

    /**
     * Computes the new configuration of the farm after a contract violation.
     * @param monitoredValue The violated monitored value.
     * @return The new configuration.
     */
    FarmConfiguration getNewConfiguration(double monitoredValue) const;

    /**
     * Changes the current farm configuration.
     * @param configuration The new configuration.
     */
    void changeConfiguration(FarmConfiguration configuration);
public:
    /**
     * Creates a farm adaptivity manager.
     * ATTENTION: Needs to be created when the farm is ready (i.e. in run* methods).
     * @param farm The farm to be managed.
     * @param adaptivityParameters The parameters to be used for adaptivity decisions.
     */
    AdaptivityManagerFarm(AdaptiveFarm<lb_t, gt_t>* farm, AdaptivityParameters* adaptivityParameters);

    /**
     * Destroyes this adaptivity manager.
     */
    ~AdaptivityManagerFarm();

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
