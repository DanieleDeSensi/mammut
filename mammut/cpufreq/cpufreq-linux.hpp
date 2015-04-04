/*
 * cpufreq-linux.hpp
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

#ifndef MAMMUT_CPUFREQ_LINUX_HPP_
#define MAMMUT_CPUFREQ_LINUX_HPP_

#include <mammut/cpufreq/cpufreq.hpp>

#include <vector>
#include <map>

namespace mammut{
namespace cpufreq{

class DomainLinux: public Domain{
public:
    DomainLinux(DomainId domainIdentifier, std::vector<topology::VirtualCore*> virtualCores);
    std::vector<Frequency> getAvailableFrequencies() const;
    std::vector<Governor> getAvailableGovernors() const;
    Frequency getCurrentFrequency() const;
    Frequency getCurrentFrequencyUserspace() const;
    bool setFrequencyUserspace(Frequency frequency) const;
    Governor getCurrentGovernor() const;
    bool setGovernor(Governor governor) const;
    void getHardwareFrequencyBounds(Frequency& lowerBound, Frequency& upperBound) const;
    bool getCurrentGovernorBounds(Frequency& lowerBound, Frequency& upperBound) const;
    bool setGovernorBounds(Frequency lowerBound, Frequency upperBound) const;
    int getTransitionLatency() const;
    Voltage getCurrentVoltage() const;
    VoltageTable getVoltageTable(bool onlyPhysicalCores = true) const;
    VoltageTable getVoltageTable(uint numVirtualCores) const;
private:
    std::vector<Governor> _availableGovernors;
    std::vector<Frequency> _availableFrequencies;
    std::vector<std::string> _paths;
    utils::Msr _msr;

    void writeToDomainFiles(const std::string& what, const std::string& where) const;
};

class CpuFreqLinux: public CpuFreq{
private:
    std::vector<Domain*> _domains;
    std::string _boostingFile;
    topology::Topology* _topology;
public:
    CpuFreqLinux();
    ~CpuFreqLinux();
    std::vector<Domain*> getDomains() const;
    bool isBoostingSupported() const;
    bool isBoostingEnabled() const;
    void enableBoosting() const;
    void disableBoosting() const;
};

}
}






#endif /* MAMMUT_CPUFREQ_LINUX_HPP_ */
