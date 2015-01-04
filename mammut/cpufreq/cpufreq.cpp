/*
 * cpufreq.cpp
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
#include <mammut/cpufreq/cpufreq-remote.hpp>
#include <mammut/cpufreq/cpufreq-remote.pb.h>
#include <mammut/utils.hpp>

#include <sstream>
#include <stdexcept>


namespace mammut{
namespace cpufreq{

Domain::Domain(DomainId domainIdentifier, std::vector<topology::VirtualCore*> virtualCores):
        _domainIdentifier(domainIdentifier), _virtualCores(virtualCores){
    ;
}

std::vector<topology::VirtualCore*> Domain::getVirtualCores() const{
    return _virtualCores;
}

std::vector<topology::VirtualCoreId> Domain::getVirtualCoresIdentifiers() const{
    std::vector<topology::VirtualCoreId> r;
    r.reserve(_virtualCores.size());
    for(size_t i = 0; i < _virtualCores.size(); i++){
        r.push_back(_virtualCores.at(i)->getVirtualCoreId());
    }
    return r;
}

DomainId Domain::getId() const{
    return _domainIdentifier;
}

std::vector<Domain*> CpuFreq::getDomains() const{
    return _domains;
}

std::vector<std::string> CpuFreq::_governorsNames = CpuFreq::initGovernorsNames();

std::vector<std::string> CpuFreq::initGovernorsNames(){
    std::vector<std::string> governorNames;
    governorNames.resize(GOVERNOR_NUM);
    governorNames.at(GOVERNOR_CONSERVATIVE) = "conservative";
    governorNames.at(GOVERNOR_ONDEMAND) = "ondemand";
    governorNames.at(GOVERNOR_USERSPACE) = "userspace";
    governorNames.at(GOVERNOR_POWERSAVE) = "powersave";
    governorNames.at(GOVERNOR_PERFORMANCE) = "performance";
    return governorNames;
}

Governor CpuFreq::getGovernorFromGovernorName(std::string governorName){
    for(unsigned int g = 0; g < GOVERNOR_NUM; g++){
        if(CpuFreq::_governorsNames.at(g).compare(governorName) == 0){
            return static_cast<Governor>(g);
        }
    }
    return GOVERNOR_NUM;
}

std::string CpuFreq::getGovernorNameFromGovernor(Governor governor){
    if(governor != GOVERNOR_NUM){
        return CpuFreq::_governorsNames.at(governor);
    }else{
        return "";
    }
}

/**
 * From a given set of virtual cores, returns only those with specified identifiers.
 * @param virtualCores The set of virtual cores.
 * @param identifiers The identifiers of the virtual cores we need.
 * @return A set of virtual cores with the specified identifiers.
 */
static std::vector<topology::VirtualCore*> filterVirtualCores(const std::vector<topology::VirtualCore*>& virtualCores,
                                                  const std::vector<topology::VirtualCoreId>& identifiers){
    std::vector<topology::VirtualCore*> r;
    for(size_t i = 0; i < virtualCores.size(); i++){
        if(utils::contains<topology::VirtualCoreId>(identifiers, virtualCores.at(i)->getVirtualCoreId())){
            r.push_back(virtualCores.at(i));
        }
    }
    return r;
}

CpuFreq::CpuFreq():_communicator(NULL), _topology(topology::Topology::local()){
#if defined(__linux__)
    if(utils::existsDirectory("/sys/devices/system/cpu/cpu0/cpufreq")){
        std::vector<std::string> output =
                utils::getCommandOutput("cat /sys/devices/system/cpu/cpu*/cpufreq/related_cpus | sort | uniq");

        std::vector<topology::VirtualCore*> vc = _topology->getVirtualCores();
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
    }else{
        throw std::runtime_error("CpuFreq: Impossible to find sysfs dir.");
    }
#else
    throw std::runtime_error("CpuFreq: Unsupported OS.");
#endif
}

CpuFreq* CpuFreq::local(){
    return new CpuFreq();
}

CpuFreq::CpuFreq(Communicator* const communicator):_communicator(communicator), _topology(topology::Topology::remote(_communicator)){
    GetDomains gd;
    GetDomainsRes r;
    _communicator->remoteCall(gd, r);

    std::vector<topology::VirtualCore*> vc = _topology->getVirtualCores();
    _domains.resize(r.domains_size());
    for(unsigned int i = 0; i < (size_t) r.domains_size(); i++){
        std::vector<topology::VirtualCoreId> virtualCoresIdentifiers;
        GetDomainsRes::Domain d = r.domains().Get(i);
        utils::pbRepeatedToVector<topology::VirtualCoreId>(d.virtual_cores_ids(), virtualCoresIdentifiers);
        _domains.at(d.id()) = new DomainRemote(_communicator, d.id(), filterVirtualCores(vc, virtualCoresIdentifiers));
    }
}

CpuFreq* CpuFreq::remote(Communicator* const communicator){
    return new CpuFreq(communicator);
}

CpuFreq::~CpuFreq(){
    utils::deleteVectorElements<Domain*>(_domains);
    topology::Topology::release(_topology);
}

void CpuFreq::release(CpuFreq* cpufreq){
    if(cpufreq){
        delete cpufreq;
    }
}

std::string CpuFreq::getModuleName(){
    // Any message defined in the .proto file is ok.
    GetAvailableFrequencies gaf;
    return utils::getModuleNameFromMessage(&gaf);
}

bool CpuFreq::processMessage(const std::string& messageIdIn, const std::string& messageIn,
                             std::string& messageIdOut, std::string& messageOut){
    {
        GetDomains gd;
        if(utils::getDataFromMessage<GetDomains>(messageIdIn, messageIn, gd)){
            GetDomainsRes r;
            DomainId domainId;
            for(unsigned int i = 0; i < _domains.size(); i++){
                domainId = _domains.at(i)->getId();
                utils::vectorToPbRepeated<uint32_t>(_domains.at(i)->getVirtualCoresIdentifiers(),
                                                    r.mutable_domains(domainId)->mutable_virtual_cores_ids());
                r.mutable_domains(domainId)->set_id(domainId);
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetAvailableFrequencies gaf;
        if(utils::getDataFromMessage<GetAvailableFrequencies>(messageIdIn, messageIn, gaf)){
            GetAvailableFrequenciesRes r;
            std::vector<Frequency> availableFrequencies = _domains.at((gaf.id()))->getAvailableFrequencies();
            utils::vectorToPbRepeated<uint32_t>(availableFrequencies, r.mutable_frequencies());
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetAvailableGovernors gag;
        if(utils::getDataFromMessage<GetAvailableGovernors>(messageIdIn, messageIn, gag)){
            GetAvailableGovernorsRes r;
            std::vector<Governor> availableGovernors = _domains.at((gag.id()))->getAvailableGovernors();
            std::vector<uint32_t> tmp;
            utils::convertVector<Governor, uint32_t>(availableGovernors, tmp);
            utils::vectorToPbRepeated<uint32_t>(tmp,
                                                r.mutable_governors());
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetCurrentFrequency gcf;
        if(utils::getDataFromMessage<GetCurrentFrequency>(messageIdIn, messageIn, gcf)){
            GetCurrentFrequencyRes r;
            if(gcf.userspace()){
                r.set_frequency(_domains.at((gcf.id()))->getCurrentFrequencyUserspace());
            }else{
                r.set_frequency(_domains.at((gcf.id()))->getCurrentFrequency());
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetCurrentGovernor gcg;
        if(utils::getDataFromMessage<GetCurrentGovernor>(messageIdIn, messageIn, gcg)){
            GetCurrentGovernorRes r;
            r.set_governor(_domains.at((gcg.id()))->getCurrentGovernor());
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        ChangeFrequency cf;
        if(utils::getDataFromMessage<ChangeFrequency>(messageIdIn, messageIn, cf)){
            Result r;
            r.set_result(_domains.at((cf.id()))->changeFrequency(cf.frequency()));
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        ChangeFrequencyBounds cfb;
        if(utils::getDataFromMessage<ChangeFrequencyBounds>(messageIdIn, messageIn, cfb)){
            Result r;
            r.set_result(_domains.at((cfb.id()))->changeFrequencyBounds(cfb.lower_bound(), cfb.upper_bound()));
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        ChangeGovernor cg;
        if(utils::getDataFromMessage<ChangeGovernor>(messageIdIn, messageIn, cg)){
            Result r;
            r.set_result(_domains.at((cg.id()))->changeGovernor((Governor) cg.governor()));
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    return false;
}

}
}
