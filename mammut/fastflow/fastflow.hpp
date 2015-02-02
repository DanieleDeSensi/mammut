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
#include <mammut/topology/topology.hpp>


namespace mammut{
namespace fastflow{

class ReconfigurableWorker: public ff::ff_node{
private:
public:
    int svc_init();
};

class ReconfigurableFarm: public ff::ff_farm{
    ;
};


/// Possible reconfiguration strategies.
typedef enum{
    RECONF_STRATEGY_NO_FREQUENCY = 0, ///< Does not reconfigure frequencies.
    RECONF_STRATEGY_OS_FREQUENCY, ///< Reconfigures the number of workers. The frequencies are managed by OS governor. The governor must be specified with 'setFrequengyGovernor' call.
    RECONF_STRATEGY_CORES_CONSERVATIVE, ///< Reconfigures frequencies and number of workers. Tries always to minimize the number of virtual cores used.
    RECONF_STRATEGY_POWER_CONSERVATIVE, ///< Reconfigures frequencies and number of workers. Tries to minimize the consumed power.
}ReconfigurationStrategy;

/// Possible mapping strategies.
typedef enum{
    MAPPING_STRATEGY_AUTO = 0, ///< Mapping strategy will be automatically chosen at runtime.
    MAPPING_STRATEGY_LINEAR, ///< Tries to keep the threads as close as possible.
    MAPPING_STRATEGY_CACHE_EFFICIENT ///< Tries to make good use of the shared caches. Particularly useful when threads have large working sets.
}MappingStrategy;


/*! \class FarmAdaptivityManager
 * \brief This class manages the adaptivity in farm based computations.
 *
 * This class manages the adaptivity in farm based computations.
 */
class FarmAdaptivityManager: public Module, public utils::Thread{
    MAMMUT_MODULE_DECL(FarmAdaptivityManager)
private:
    cpufreq::CpuFreq* _cpufreq;
    energy::Energy* _energy;
    topology::Topology* _topology;
    ff::ff_farm* _farm;
    ReconfigurationStrategy _reconfigurationStrategy;
    cpufreq::Governor _frequencyGovernor;
    uint32_t _numSamples;
    uint32_t _samplingInterval;
    double _underloadThresholdFarm;
    double _overloadThresholdFarm;
    double _underloadThresholdWorker;
    double _overloadThresholdWorker;
    bool _migrateCollector;
    uint32_t _stabilizationPeriod;

    /**
     * Creates a farm adaptivity manager.
     * @param communicator A pointer to a communicator.
     *        If NULL, the other modules will be instantiated as
     *        local modules.
     */
    FarmAdaptivityManager(Communicator* const communicator);

    /**
     * Prepares the frequencies and governors for running.
     */
    void initCpuFreq();

    /**
     * Prepares the mapping of the threads on virtual cores.
     */
    void initMapping();
public:
    /**
     * Destroys this adaptivity manager.
     */
    ~FarmAdaptivityManager();

    /**
     * Checks if a specific reconfiguration strategy is available.
     * @param strategy The reconfiguration strategy.
     * @return True if the strategy is available, false otherwise.
     */
    bool isReconfigurationStrategyAvailable(ReconfigurationStrategy strategy);

    /**
     * Returns the reconfiguration strategy.
     * @return The reconfiguration strategy.
     */
    ReconfigurationStrategy getReconfigurationStrategy() const;

    /**
     * Sets the reconfiguration strategy.
     * @param reconfigurationStrategy The reconfiguration strategy.
     */
    void setReconfigurationStrategy(ReconfigurationStrategy reconfigurationStrategy);

    /**
     * Checks if a specific OS frequency governor is available.
     * @param governor The frequency governor.
     * @return true if the governor is available, false otherwise.
     */
    bool isFrequencyGovernorAvailable(cpufreq::Governor governor);

    /**
     * Returns the frequency governor that has been set for
     * RECONF_STRATEGY_OS_FREQUENCY reconfiguration strategy.
     * @return The frequency governor that has been set for
     *         RECONF_STRATEGY_OS_FREQUENCY reconfiguration strategy.
     */
    cpufreq::Governor getFrequencyGovernor() const;

    /**
     * Sets the frequency governor to be used by the OS in
     * RECONF_STRATEGY_OS_FREQUENCY reconfiguration strategy.
     * @param frequencyGovernor The frequency governor to be used by the OS in
     *         RECONF_STRATEGY_OS_FREQUENCY reconfiguration strategy. On different
     *         strategies, this value will be ignored.
     */
    void setFrequencyGovernor(cpufreq::Governor frequencyGovernor);

    /**
     * Runs this manager. setFarm must be called before calling this method.
     */
    void run();

    /**
     * Stops this manager;
     */
    void stop();

    /**
     * Returns the farm managed by this instance.
     * @return The farm managed by this instance.
     */
    const ff::ff_farm*& getFarm() const;

    /**
     * Sets the farm to be managed by this instance.
     * @param farm The farm to be managed by this instance.
     */
    void setFarm(const ff::ff_farm*& farm);

    /**
     * Returns the underload threshold for the entire farm.
     * @return The underload threshold for the entire farm.
     */
    double getUnderloadThresholdFarm() const;

    /**
     * Sets the underload threshold for the entire farm.
     * @param loadThresholdDownFarm The underload threshold for the entire farm.
     *        It must be included in the range [0, 100].
     */
    void setUnderloadThresholdFarm(double underloadThresholdFarm);

    /**
     * Returns the underload threshold for a single worker.
     * @return The underload threshold for a single worker.
     */
    double getUnderloadThresholdWorker() const;

    /**
     * Sets the underload threshold for a single worker.
     * @param loadThresholdDownWorker The underload threshold for a single worker.
     *        It must be included in the range [0, 100].
     */
    void setUnderloadThresholdWorker(double underloadThresholdWorker);

    /**
     * Returns the overload threshold for the entire farm.
     * @return The overload threshold for the entire farm.
     */
    double getOverloadThresholdFarm() const;

    /**
     * Sets the overload threshold for the entire farm.
     * @param loadThresholdUpFarm The overload threshold for the entire farm.
     *        It must be included in the range [0, 100].
     */
    void setOverloadThresholdFarm(double overloadThresholdFarm);

    /**
     * Returns the overload threshold for a single worker.
     * @return The overload threshold for a single worker.
     */
    double getOverloadThresholdWorker() const;

    /**
     * Sets the overload threshold for a single worker.
     * @param loadThresholdUpWorker The overload threshold for a single worker.
     *        It must be included in the range [0, 100].
     */
    void setOverloadThresholdWorker(double overloadThresholdWorker);

    /**
     * Return the current value of migrateCollector flag.
     * @return The current value of migrateCollector flag.
     */
    bool isMigrateCollector() const;

    /**
     * Sets the value of migrateCollector flag.
     * @param migrateCollector If true, when a reconfiguration
     *        occurs, the collector is migrated closer to the workers.
     */
    void setMigrateCollector(bool migrateCollector);

    /**
     *  Return the number of samples used to take reconfiguration decisions.
     *  @return The number of samples used to take reconfiguration decisions.
     */
    uint32_t getNumSamples() const;

    /**
     * Sets the number of samples to be used to take reconfiguration decisions.
     * @param numSamples The number of samples to be used to take reconfiguration decisions.
     */
    void setNumSamples(uint32_t numSamples);

    /**
     * Return the length of the sampling interval (in seconds) over which the
     * reconfiguration decisions are taken.
     * @return The length of the sampling interval (in seconds) over which the
     *         reconfiguration decisions are taken.
     */
    uint32_t getSamplingInterval() const;

    /**
     * Sets the length of the sampling interval (in seconds) over which the
     * reconfiguration decisions are taken.
     * @param samplingInterval The length of the sampling interval (in seconds)
     *        over which the reconfiguration decisions are taken.
     */
    void setSamplingInterval(uint32_t samplingInterval);

    /**
     * Returns the duration of the stabilization period.
     * @return The duration of the stabilization period. It corresponds
     *         to the minimum number of seconds that must elapse between two
     *         successive reconfiguration.
     */
    uint32_t getStabilizationPeriod() const;

    /**
     * Sets the duration of the stabilization period.
     * @param stabilizationPeriod The minimum number of seconds that must
     *        elapse between two successive reconfiguration.
     */
    void setStabilizationPeriod(uint32_t stabilizationPeriod);
};

}
}

#endif /* MAMMUT_FASTFLOW_HPP_ */
