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
    for(size_t i = 0; i < virtualCores.size(); i++){
        _paths.push_back("/sys/devices/system/cpu/cpu" + utils::intToString(virtualCores.at(0)->getVirtualCoreId()) + "/cpufreq/");
    }

    std::ifstream freqFile((_paths.at(0) + "scaling_available_frequencies").c_str());
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
    std::ifstream govFile((_paths.at(0) + "scaling_available_governors").c_str());
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

void DomainLinux::writeToDomainFiles(const std::string& what, const std::string& where) const{
    for(size_t i = 0; i < _paths.size(); i++){
        utils::executeCommand("echo " + what + " | tee " + _paths.at(i) + where + " > /dev/null");
    }
}

std::vector<Frequency> DomainLinux::getAvailableFrequencies() const{
    return _availableFrequencies;
}

std::vector<Governor> DomainLinux::getAvailableGovernors() const{
    return _availableGovernors;
}

Frequency DomainLinux::getCurrentFrequency() const{
    std::string fileName = _paths.at(0) + "scaling_cur_freq";
    return utils::stringToInt(utils::readFirstLineFromFile(fileName));
}

Frequency DomainLinux::getCurrentFrequencyUserspace() const{
    switch(getCurrentGovernor()){
        case GOVERNOR_USERSPACE:{
            std::string fileName = _paths.at(0) + "scaling_setspeed";
            return utils::stringToInt(utils::readFirstLineFromFile(fileName));
        }
        default:{
            return 0;
        }
    }
}

Governor DomainLinux::getCurrentGovernor() const{
    std::string fileName = _paths.at(0) + "scaling_governor";
    return CpuFreq::getGovernorFromGovernorName(utils::readFirstLineFromFile(fileName));
}

bool DomainLinux::changeFrequency(Frequency frequency) const{
    switch(getCurrentGovernor()){
        case GOVERNOR_USERSPACE:{
            if(!mammut::utils::contains(getAvailableFrequencies(), frequency)){
                return false;
            }
            writeToDomainFiles(utils::intToString(frequency), "scaling_setspeed");
            return true;
        }
        default:{
            return false;
        }
    }
}

void DomainLinux::getHardwareFrequencyBounds(Frequency& lowerBound, Frequency& upperBound) const{
    lowerBound = utils::stringToInt(utils::readFirstLineFromFile(_paths.at(0) + "cpuinfo_min_freq"));
    upperBound = utils::stringToInt(utils::readFirstLineFromFile(_paths.at(0) + "cpuinfo_max_freq"));
}

bool DomainLinux::getCurrentGovernorBounds(Frequency& lowerBound, Frequency& upperBound) const{
    switch(getCurrentGovernor()){
        case GOVERNOR_ONDEMAND:
        case GOVERNOR_CONSERVATIVE:
        case GOVERNOR_PERFORMANCE:
        case GOVERNOR_POWERSAVE:{
            lowerBound = utils::stringToInt(utils::readFirstLineFromFile(_paths.at(0) + "scaling_min_freq"));
            upperBound = utils::stringToInt(utils::readFirstLineFromFile(_paths.at(0) + "scaling_max_freq"));
            return true;
        }
        default:{
            return false;
        }
    }
}

bool DomainLinux::changeGovernorBounds(Frequency lowerBound, Frequency upperBound) const{
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

            writeToDomainFiles(utils::intToString(lowerBound), "scaling_min_freq");
            writeToDomainFiles(utils::intToString(upperBound), "scaling_max_freq");
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

    writeToDomainFiles(CpuFreq::getGovernorNameFromGovernor(governor), "scaling_governor");
    return true;
}

CpuFreqLinux::CpuFreqLinux():
    _boostingFile("/sys/devices/system/cpu/cpufreq/boost"){
    if(utils::existsDirectory("/sys/devices/system/cpu/cpu0/cpufreq")){
        topology::Topology* topology = topology::Topology::local();
        std::vector<std::string> output;

        /** If freqdomain_cpus file are present, we must consider them instead of related_cpus. **/
        std::string domainsFiles;
        if(utils::existsFile("/sys/devices/system/cpu/cpu0/cpufreq/freqdomain_cpus")){
            domainsFiles = "freqdomain_cpus";
        }else{
            domainsFiles = "related_cpus";
        }

        output = utils::getCommandOutput("cat /sys/devices/system/cpu/cpu*/cpufreq/" + domainsFiles + " | sort | uniq");

        std::vector<topology::VirtualCore*> vc = topology->getVirtualCores();
        _domains.resize(output.size());
        for(size_t i = 0; i < output.size(); i++){
            /** Converts the line to a vector of virtual cores identifiers. **/
            std::stringstream ss(output.at(i).c_str());
            std::vector<topology::VirtualCoreId> virtualCoresIdentifiers;
            topology::VirtualCoreId num;
            while(ss >> num){
                virtualCoresIdentifiers.push_back(num);
            }
            /** Creates a domain based on the vector of cores identifiers. **/
            _domains.at(i) = new DomainLinux(i, filterVirtualCores(vc, virtualCoresIdentifiers));
        }
        topology::Topology::release(topology);
    }else{
        throw std::runtime_error("CpuFreq: Impossible to find sysfs dir.");
    }
}

CpuFreqLinux::~CpuFreqLinux(){
    utils::deleteVectorElements<Domain*>(_domains);
}

std::vector<Domain*> CpuFreqLinux::getDomains() const{
    return _domains;
}

bool CpuFreqLinux::isBoostingSupported() const{
    //TODO: Se esiste il file è abilitabile dinamicamente. Potrebbe esserci boosting anche se il file non esiste?
    return utils::existsFile(_boostingFile);
}

bool CpuFreqLinux::isBoostingEnabled() const{
    if(isBoostingSupported()){
        if(utils::stringToInt(utils::readFirstLineFromFile(_boostingFile))){
            return true;
        }
    }
    return false;
}

void CpuFreqLinux::enableBoosting() const{
    utils::executeCommand("echo 1 > " + _boostingFile);
}

void CpuFreqLinux::disableBoosting() const{
    utils::executeCommand("echo 0 > " + _boostingFile);
}

}
}
