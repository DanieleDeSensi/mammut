/*
 * cpufreq-remote.hpp
 *
 * Created on: 12/11/2014
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

#ifndef CPUFREQ_REMOTE_HPP_
#define CPUFREQ_REMOTE_HPP_

#include <mammut/cpufreq/cpufreq.hpp>

namespace mammut{
namespace cpufreq{

class DomainRemote: public Domain{
public:
    DomainRemote(Communicator* const communicator, DomainId domainIdentifier, std::vector<topology::VirtualCore*> virtualCores);
    std::vector<Frequency> getAvailableFrequencies() const;
    std::vector<Governor> getAvailableGovernors() const;
    Frequency getCurrentFrequency() const;
    Frequency getCurrentFrequencyUserspace() const;
    Governor getCurrentGovernor() const;
    bool changeFrequency(Frequency frequency) const;
    bool changeFrequencyBounds(Frequency lowerBound, Frequency upperBound) const;
    bool changeGovernor(Governor governor) const;
private:
    Frequency getCurrentFrequency(bool userspace) const;
    Communicator* const _communicator;
};

}
}

#endif /* CPUFREQ_REMOTE_HPP_ */
