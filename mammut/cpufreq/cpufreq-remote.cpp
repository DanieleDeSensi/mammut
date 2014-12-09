/*
 * cpufreq-remote.cpp
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

#include <mammut/cpufreq/cpufreq.hpp>
#include <mammut/cpufreq/cpufreq-remote.hpp>
#include <mammut/cpufreq/cpufreq-remote.pb.h>
#include <mammut/utils.hpp>

namespace mammut{
namespace cpufreq{

DomainRemote::DomainRemote(Communicator* const communicator, DomainId domainIdentifier, std::vector<topology::VirtualCore*> virtualCores):
                           Domain(domainIdentifier, virtualCores), _communicator(communicator){
    ;
}

std::vector<Frequency> DomainRemote::getAvailableFrequencies() const{
    GetAvailableFrequencies gaf;
    GetAvailableFrequenciesRes r;
    std::vector<Frequency> availableFrequencies;

    gaf.set_id(getId());
    _communicator->remoteCall(gaf, r);
    utils::pbRepeatedToVector<uint32_t>(r.frequencies(), availableFrequencies);
    return availableFrequencies;
}

std::vector<Governor> DomainRemote::getAvailableGovernors() const{
    GetAvailableGovernors gag;
    GetAvailableGovernorsRes r;
    std::vector<Governor> availableGovernors;

    gag.set_id(getId());
    _communicator->remoteCall(gag, r);
    std::vector<uint32_t> tmp;
    utils::convertVector<uint32_t, Governor>(utils::pbRepeatedToVector<uint32_t>(r.governors(), tmp),
                                             availableGovernors);
    return availableGovernors;
}

Frequency DomainRemote::getCurrentFrequency(bool userspace) const{
    GetCurrentFrequency gcf;
    GetCurrentFrequencyRes r;

    gcf.set_id(getId());
    gcf.set_userspace(userspace);
    _communicator->remoteCall(gcf, r);
    return r.frequency();
}

Frequency DomainRemote::getCurrentFrequency() const{
    return getCurrentFrequency(false);
}

Frequency DomainRemote::getCurrentFrequencyUserspace() const{
    return getCurrentFrequency(true);
}

Governor DomainRemote::getCurrentGovernor() const{
    GetCurrentGovernor gcg;
    GetCurrentGovernorRes r;

    gcg.set_id(getId());
    _communicator->remoteCall(gcg, r);
    return static_cast<Governor>(r.governor());
}

bool DomainRemote::changeFrequency(Frequency frequency) const{
    ChangeFrequency cf;
    Result r;
    cf.set_id(getId());
    cf.set_frequency(frequency);
    _communicator->remoteCall(cf, r);
    return r.result();
}

bool DomainRemote::changeFrequencyBounds(Frequency lowerBound, Frequency upperBound) const{
    ChangeFrequencyBounds cflb;
    Result r;
    cflb.set_id(getId());
    cflb.set_lower_bound(lowerBound);
    cflb.set_upper_bound(upperBound);
    _communicator->remoteCall(cflb, r);
    return r.result();
}

bool DomainRemote::changeGovernor(Governor governor) const{
    ChangeGovernor cg;
    Result r;
    cg.set_id(getId());
    cg.set_governor(governor);
    _communicator->remoteCall(cg, r);
    return r.result();
}

}
}
