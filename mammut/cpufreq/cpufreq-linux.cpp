/*
 * cpufreq-linux.cpp
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

#include <mammut/cpufreq/cpufreq.hpp>
#include <mammut/cpufreq/cpufreq-linux.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace mammut{
namespace cpufreq{

DomainLinux::DomainLinux(DomainId domainIdentifier, std::vector<topology::VirtualCore*> virtualCores):Domain(domainIdentifier, virtualCores){
    /** Reads available frequecies. **/
    unsigned long frequency;
    _path = "/sys/devices/system/cpu/cpu" + utils::intToString(getVirtualCores().at(0)->getVirtualCoreId()) + "/cpufreq/";
    std::ifstream freqFile((_path + "scaling_available_frequencies").c_str());
    if(freqFile.is_open()){
        while(freqFile >> frequency){
            _availableFrequencies.push_back(frequency);;
        }
        std::sort(_availableFrequencies.begin(), _availableFrequencies.end());
        freqFile.close();
    }else{
        throw std::runtime_error("Impossible to open scaling_available_frequencies file.");
    }

    /** Reads available governors. **/
    std::string governorName;
    Governor governor;
    std::ifstream govFile((_path + "scaling_available_governors").c_str());
    if(govFile.is_open()){
        while(govFile >> governorName){
            governor = CpuFreq::getGovernorFromGovernorName(governorName);
            if(governor != GOVERNOR_NUM){
                _availableGovernors.push_back(governor);
            }
        }
        govFile.close();
    }else{
        throw std::runtime_error("Impossible to open scaling_available_governors file.");
    }
}

std::vector<Frequency> DomainLinux::getAvailableFrequencies() const{
    return _availableFrequencies;
}

std::vector<Governor> DomainLinux::getAvailableGovernors() const{
    return _availableGovernors;
}

Frequency DomainLinux::getCurrentFrequency() const{
    std::string fileName = _path + "scaling_cur_freq";
    return utils::stringToInt(utils::readFirstLineFromFile(fileName));
}

Frequency DomainLinux::getCurrentFrequencyUserspace() const{
    switch(getCurrentGovernor()){
        case GOVERNOR_USERSPACE:{
            std::string fileName = _path + "scaling_setspeed";
            return utils::stringToInt(utils::readFirstLineFromFile(fileName));
        }
        default:{
            return 0;
        }
    }
}

Governor DomainLinux::getCurrentGovernor() const{
    std::string fileName = _path + "scaling_governor";
    return CpuFreq::getGovernorFromGovernorName(utils::readFirstLineFromFile(fileName));
}

bool DomainLinux::changeFrequency(Frequency frequency) const{
    switch(getCurrentGovernor()){
        case GOVERNOR_USERSPACE:{
            if(!mammut::utils::contains(getAvailableFrequencies(), frequency)){
                return false;
            }
            std::ostringstream ss;
            ss << "cpufreq-set -f " << frequency <<
                             " -c " << getVirtualCores().at(0)->getVirtualCoreId() <<
                             " -r";
            utils::executeCommand(ss.str());
            return true;
        }
        default:{
            return false;
        }
    }
}


bool DomainLinux::changeFrequencyBounds(Frequency lowerBound, Frequency upperBound) const{
    switch(getCurrentGovernor()){
        case GOVERNOR_ONDEMAND:
        case GOVERNOR_CONSERVATIVE:
        case GOVERNOR_PERFORMANCE:
        case GOVERNOR_POWERSAVE:{
            if(!mammut::utils::contains(getAvailableFrequencies(), lowerBound) ||
               !mammut::utils::contains(getAvailableFrequencies(), upperBound) ||
               lowerBound > upperBound){
                 return false;
             }
             std::ostringstream ss;
             ss << "cpufreq-set -d " << lowerBound <<
                              " -c " << getVirtualCores().at(0)->getVirtualCoreId() <<
                              " -r";
             utils::executeCommand(ss.str());
             ss.clear();

             ss << "cpufreq-set -u " << upperBound <<
                              " -c " << getVirtualCores().at(0)->getVirtualCoreId() <<
                              " -r";
             utils::executeCommand(ss.str());
             return true;
        }
        default:{
            return false;
        }
    }
}

bool DomainLinux::changeGovernor(Governor governor) const{
    if(!mammut::utils::contains(_availableGovernors, governor)){
        return false;
    }
    std::ostringstream ss;
    ss << "cpufreq-set -g " << CpuFreq::getGovernorNameFromGovernor(governor) <<
                     " -c " << getVirtualCores().at(0)->getVirtualCoreId() <<
                     " -r";
    utils::executeCommand(ss.str());
    return true;
}

}
}
