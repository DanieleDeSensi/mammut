/*
 * cpufreq.hpp
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

#ifndef MAMMUT_CPUFREQ_HPP_
#define MAMMUT_CPUFREQ_HPP_

#include <mammut/communicator.hpp>
#include <mammut/module.hpp>
#include <mammut/topology/topology.hpp>
#include <mammut/utils.hpp>

#include <map>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

namespace mammut{
namespace cpufreq{

typedef enum{
    GOVERNOR_CONSERVATIVE,
    GOVERNOR_ONDEMAND,
    GOVERNOR_USERSPACE,
    GOVERNOR_POWERSAVE,
    GOVERNOR_PERFORMANCE,
    GOVERNOR_NUM
}Governor;

typedef uint32_t Frequency;
typedef double Voltage;
typedef uint32_t DomainId;

typedef std::pair<topology::VirtualCoreId, Frequency> VoltageTableKey;
typedef std::map<VoltageTableKey, Voltage> VoltageTable;
typedef std::map<VoltageTableKey, Voltage>::const_iterator VoltageTableIterator;

/**
 * Represents a rollback point. It can be used to bring
 * the domain to a previous state.
 */
struct RollbackPoint{
    DomainId domainId;
    Frequency frequency;
    Frequency lowerBound;
    Frequency upperBound;
    Governor governor;
};

/**
 * Represents a set of virtual cores related between each other.
 * When the frequency/governor changes for one core in the domain,
 * it changes for the other cores in the same domain too.
 */
class Domain{
    const DomainId _domainIdentifier;
protected:
    const std::vector<topology::VirtualCore*> _virtualCores;

    Domain(DomainId domainIdentifier, std::vector<topology::VirtualCore*> virtualCores);
public:
    virtual inline ~Domain(){;}

    /**
     * Returns the virtual cores inside the domain.
     * @return The virtual cores inside the domain.
     */
    std::vector<topology::VirtualCore*> getVirtualCores() const;

    /**
     * Returns the identifiers of the virtual cores inside the domain.
     * @return The identifiers of the virtual cores inside the domain.
     */
    std::vector<topology::VirtualCoreId> getVirtualCoresIdentifiers() const;

    /**
     * Checks if this domain contains a specific virtual core.
     * @param virtualCore The virtual core.
     * @return true if this domain contains the virtual core,
     *         false otherwise.
     */
    bool contains(const topology::VirtualCore* virtualCore) const;

    /**
     * Returns the identifier of the domain.
     * @return The identifier of the domain.
     */
    DomainId getId() const;

    /**
     * Returns a rollback point. It can be used to bring the domain
     * back to the point when this function is called.
     * @return A rollback point.
     */
    RollbackPoint getRollbackPoint() const;

    /**
     * Bring the domain to a rollback point.
     * @param rollbackPoint A rollback point.
     */
    void rollback(const RollbackPoint& rollbackPoint) const;

    /**
     * Gets the frequency steps (in KHz) available (sorted from lowest to highest).
     * @return The frequency steps (in KHz) available (sorted from lowest to highest).
     **/
    virtual std::vector<Frequency> getAvailableFrequencies() const = 0;

    /**
     * Gets the governors available.
     * @return The governors available.
     **/
    virtual std::vector<Governor> getAvailableGovernors() const = 0;

    /**
     * Checks the availability of a specific governor.
     * @param governor The governor.
     * @return True if the governor is available, false otherwise.
     */
    bool isGovernorAvailable(Governor governor) const;

    /**
     * Gets the current frequency.
     * @return The current frequency.
     */
    virtual Frequency getCurrentFrequency() const = 0;

    /**
     * Gets the current frequency set by userspace governor.
     * @return The current frequency if the current governor is userspace,
     *         a meaningless value otherwise.
     */
    virtual Frequency getCurrentFrequencyUserspace() const = 0;

    /**
     * Change the running frequency.
     * @param frequency The frequency to be set (specified in kHZ).
     * @return true if the frequency is valid and the governor is userspace,
     *              false otherwise.
     **/
    virtual bool setFrequencyUserspace(Frequency frequency) const = 0;

    /**
     * Sets the highest userspace frequency for this domain.
     * @return false if the current governor is not userspace.
     */
    bool setHighestFrequencyUserspace() const;

    /**
     * Sets the lowest userspace frequency for this domain.
     * @return false if the current governor is not userspace.
     */
    bool setLowestFrequencyUserspace() const;

    /**
     * Gets the current governor.
     * @return The current governor.
     */
    virtual Governor getCurrentGovernor() const = 0;

    /**
     * Changes the governor.
     * @param governor The identifier of the governor.
     * @return true if the governor is valid, false otherwise.
     **/
    virtual bool setGovernor(Governor governor) const = 0;

    /**
     * Gets the hardware frequency bounds.
     * @param lowerBound The hardware frequency lower bound (specified in kHZ).
     * @param upperBound The hardware frequency upper bound (specified in kHZ).
     **/
    virtual void getHardwareFrequencyBounds(Frequency& lowerBound, Frequency& upperBound) const = 0;

    /**
     * Gets the current frequency bounds for the governor.
     * @param lowerBound The current frequency lower bound (specified in kHZ).
     * @param upperBound The current frequency upper bound (specified in kHZ).
     * @return true if the governor is not userspace, false otherwise.
     **/
    virtual bool getCurrentGovernorBounds(Frequency& lowerBound, Frequency& upperBound) const = 0;

    /**
     * Change the frequency bounds of the governor.
     * @param lowerBound The new frequency lower bound (specified in kHZ).
     * @param upperBound The new frequency upper bound (specified in kHZ).
     * @return true if the bounds are valid and the governor is not userspace, false otherwise.
     **/
    virtual bool setGovernorBounds(Frequency lowerBound, Frequency upperBound) const = 0;

    /**
     * Returns the frequency transition latency (nanoseconds).
     * @return The frequency transition latency (nanoseconds). -1 will be returned if the
     *         latency is unknown.
     */
    virtual int getTransitionLatency() const = 0;

    /**
     * Returns the current voltage of this domain.
     * @return The current voltage of this domain.
     *         It returns 0 if is not possible to read
     *         the current voltage on this domain.
     */
    virtual Voltage getCurrentVoltage() const = 0;


    /**
     * Returns the voltage table of this domain.
     * The voltage table is a map where for each pair
     * <N, F> is associated a voltage V.
     * N is the number of virtual cores running at frequency
     * F at 100% of load.
     * NOTE: This call may block the caller for some seconds/minutes.
     * @param onlyPhysicalCores If true, only physical cores will be considered.
     * @return The voltage table of this domain. If voltages cannot be read,
     *         the table will be empty.
     */
    virtual VoltageTable getVoltageTable(bool onlyPhysicalCores = true) const = 0;

    /**
     * Returns the voltage table of this domain. For a specific
     * number of virtual cores N.
     * The voltage table is a map where for each pair
     * <N, F> is associated a voltage V.
     * N is the number of virtual cores running at frequency
     * F at 100% of load.
     * NOTE: This call may block the caller for some seconds/minutes.
     * @param numVirtualCores The number of virtual cores.
     * @return The voltage table of this domain. If voltages cannot be read,
     *         the table will be empty.
     */
    virtual VoltageTable getVoltageTable(uint numVirtualCores) const = 0;
};

class CpuFreq: public Module{
    MAMMUT_MODULE_DECL(CpuFreq)
private:
    bool processMessage(const std::string& messageIdIn, const std::string& messageIn,
                                    std::string& messageIdOut, std::string& messageOut);
protected:
    virtual ~CpuFreq(){;}
    /**
     * From a given set of virtual cores, returns only those with specified identifiers.
     * @param virtualCores The set of virtual cores.
     * @param identifiers The identifiers of the virtual cores we need.
     * @return A set of virtual cores with the specified identifiers.
     */
    static std::vector<topology::VirtualCore*> filterVirtualCores(const std::vector<topology::VirtualCore*>& virtualCores,
                                                      const std::vector<topology::VirtualCoreId>& identifiers);
public:
    /**
     * Gets the domains division of the cores.
     * @return A vector of domains.
     */
    virtual std::vector<Domain*> getDomains() const = 0;

    /**
     * Gets the domain of a specified virtual core.
     * @param virtualCore The virtual core.
     * @return The domain of the virtual core.
     */
    Domain* getDomain(const topology::VirtualCore* virtualCore) const;

    /**
     * Given a set of virtual cores, returns the domains to which these virtual cores belong.
     * @param virtualCores The set of virtual cores.
     * @return A vector of domains.
     */
    std::vector<Domain*> getDomains(const std::vector<topology::VirtualCore*>& virtualCores) const;

    /**
     * Given a set of virtual cores, returns the a vector of domains.
     * The domains in this vector are only those who have all their virtual cores contained
     * in the specified set. Accordingly, for each element D in the returned vector,
     * D->getVirtualCores() will return a subset of virtualCores parameter.
     * @param virtualCores The set of virtual cores.
     * @return A vector of domains.
     */
    std::vector<Domain*> getDomainsComplete(const std::vector<topology::VirtualCore*>& virtualCores) const;

    /**
     * Returns a rollback point for each domain. They can be used to bring
     * the domains back to the point when this function is called.
     * @return A vector of rollback points.
     */
    std::vector<RollbackPoint> getRollbackPoints() const;

    /**
     * Bring the domains to the respective rollback points.
     * @param rollbackPoints A vector of rollback points.
     */
    void rollback(const std::vector<RollbackPoint>& rollbackPoints) const;

    /**
     * Checks the availability of a specific governor.
     * @param governor The governor.
     * @return True if the governor is available for all domains, false otherwise.
     */
    bool isGovernorAvailable(Governor governor) const;

    /**
     * Returns true if frequency boosting is supported.
     * @return True if frequency boosting is supported, false otherwise.
     */
    virtual bool isBoostingSupported() const = 0;

    /**
     * Returns true if frequency boosting is enabled.
     * @return True if frequency boosting is enabled, false otherwise.
     */
    virtual bool isBoostingEnabled() const = 0;

    /**
     * Enables frequency boosting.
     */
    virtual void enableBoosting() const = 0;

    /**
     * Disabled frequency boosting.
     */
    virtual void disableBoosting() const = 0;

    /**
     * Returns the governor name associated to a specific identifier.
     * @param governor The identifier of the governor.
     * @return The governor name associated to governor.
     */
    static std::string getGovernorNameFromGovernor(Governor governor);

    /**
     * Returns the governor identifier starting from the name of the governor.
     * @param governorName The name of the governor.
     * @return The identifier associated to governorName, or GOVERNOR_NUM
     *         if no association is present.
     */
    static Governor getGovernorFromGovernorName(const std::string& governorName);
};


/**
 * Loads a voltage table from a file.
 * @param voltageTable The loaded voltage table.
 * @param fileName The name of the file containing the voltage table.
 */
void loadVoltageTable(VoltageTable& voltageTable, std::string fileName);

/**
 * Dumps the voltage table on a file.
 * @param voltageTable The voltage table.
 * @param fileName The name of the file where the table must be dumped.
 */
void dumpVoltageTable(const VoltageTable& voltageTable, std::string fileName);

}
}

#endif /* MAMMUT_CPUFREQ_HPP_ */
