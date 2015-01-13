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

#ifndef CPUFREQ_LINUX_HPP_
#define CPUFREQ_LINUX_HPP_

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
    Governor getCurrentGovernor() const;
    bool changeFrequency(Frequency frequency) const;
    bool changeFrequencyBounds(Frequency lowerBound, Frequency upperBound) const;
    bool changeGovernor(Governor governor) const;
private:
    std::vector<Governor> _availableGovernors;
    std::vector<Frequency> _availableFrequencies;
    std::string _path;
};

class CpuFreqLinux: public CpuFreq{
private:
    std::vector<Domain*> _domains;
    std::string _boostingFile;
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






#endif /* CPUFREQ_LINUX_HPP_ */
