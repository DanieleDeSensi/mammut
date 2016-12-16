#include "../cpufreq/cpufreq.hpp"
#include "../cpufreq/cpufreq-linux.hpp"
#ifdef MAMMUT_REMOTE
#include "../cpufreq/cpufreq-remote.hpp"
#include "../cpufreq/cpufreq-remote.pb.h"
#endif
#include "../utils.hpp"

#include "fstream"
#include "sstream"
#include "stdexcept"

#include "iostream"

namespace mammut{
namespace cpufreq{

template<> char const* utils::enumStrings<Governor>::data[] = {
    "conservative",
    "ondemand",
    "interactive",
    "userspace",
    "powersave",
    "performance"
};


Domain::Domain(DomainId domainIdentifier, std::vector<topology::VirtualCore*> virtualCores):
        _domainIdentifier(domainIdentifier), _virtualCores(virtualCores){
    ;
}

std::vector<topology::VirtualCore*> Domain::getVirtualCores() const{
    return _virtualCores;
}

std::vector<topology::VirtualCoreId> Domain::getVirtualCoresIdentifiers() const{
    std::vector<topology::VirtualCoreId> r;
    for(size_t i = 0; i < _virtualCores.size(); i++){
        r.push_back(_virtualCores.at(i)->getVirtualCoreId());
    }
    return r;
}

bool Domain::contains(const topology::VirtualCore* virtualCore) const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        if(_virtualCores.at(i)->getVirtualCoreId() == virtualCore->getVirtualCoreId()){
            return true;
        }
    }
    return false;
}

DomainId Domain::getId() const{
    return _domainIdentifier;
}

RollbackPoint Domain::getRollbackPoint() const{
    RollbackPoint rp;
    rp.domainId = _domainIdentifier;
    rp.governor = getCurrentGovernor();
    if(rp.governor == GOVERNOR_USERSPACE){
        rp.frequency = getCurrentFrequencyUserspace();
    }else{
        getCurrentGovernorBounds(rp.lowerBound, rp.upperBound);
    }
    return rp;
}


void Domain::rollback(const RollbackPoint& rollbackPoint) const{
    if(rollbackPoint.domainId != _domainIdentifier){
        throw std::runtime_error("Domain: rollback called on an invalid rollbackPoint.");
    }
    if(!setGovernor(rollbackPoint.governor)){
        throw std::runtime_error("Domain: Impossible to rollback the domain to governor: " +
                                  CpuFreq::getGovernorNameFromGovernor(rollbackPoint.governor));
    }
    if(rollbackPoint.governor == GOVERNOR_USERSPACE){
        if(!setFrequencyUserspace(rollbackPoint.frequency)){
            throw std::runtime_error("Domain: Impossible to rollback the domain to frequency: " +
                                      utils::intToString(rollbackPoint.frequency));
        }
    }else{
        if(!setGovernorBounds(rollbackPoint.lowerBound, rollbackPoint.upperBound)){
            throw std::runtime_error("Domain: Impossible to rollback the domain to bounds: " +
                                     utils::intToString(rollbackPoint.lowerBound) + " " +
                                     utils::intToString(rollbackPoint.upperBound));
        }
    }
}

bool Domain::isGovernorAvailable(Governor governor) const{
    return utils::contains(getAvailableGovernors(), governor);
}

bool Domain::setHighestFrequencyUserspace() const{
    std::vector<Frequency> availableFrequencies = getAvailableFrequencies();
    if(!availableFrequencies.size()){
        return false;
    }else{
        return setFrequencyUserspace(availableFrequencies.back());
    }
}

bool Domain::setLowestFrequencyUserspace() const{
    std::vector<Frequency> availableFrequencies = getAvailableFrequencies();
    if(!availableFrequencies.size()){
        return false;
    }else{
        return setFrequencyUserspace(availableFrequencies.front());
    }
}

Governor CpuFreq::getGovernorFromGovernorName(const std::string& governorName){
    Governor g;
    return utils::stringToEnum(governorName, g);
}

std::string CpuFreq::getGovernorNameFromGovernor(Governor governor){
    return utils::enumToString(governor);
}

CpuFreq* CpuFreq::local(){
#if defined(__linux__)
    return new CpuFreqLinux();
#else
    throw std::runtime_error("CpuFreq: Unsupported OS.");
#endif
}

/**
 * From a given set of virtual cores, returns only those with specified identifiers.
 * @param virtualCores The set of virtual cores.
 * @param identifiers The identifiers of the virtual cores we need.
 * @return A set of virtual cores with the specified identifiers.
 */
std::vector<topology::VirtualCore*> CpuFreq::filterVirtualCores(const std::vector<topology::VirtualCore*>& virtualCores,
                                                  const std::vector<topology::VirtualCoreId>& identifiers){
    std::vector<topology::VirtualCore*> r;
    for(size_t i = 0; i < virtualCores.size(); i++){
        if(utils::contains<topology::VirtualCoreId>(identifiers, virtualCores.at(i)->getVirtualCoreId())){
            r.push_back(virtualCores.at(i));
        }
    }

    return r;
}

CpuFreq* CpuFreq::remote(Communicator* const communicator){
#ifdef MAMMUT_REMOTE
    return new CpuFreqRemote(communicator);
#else
    throw std::runtime_error("You need to define MAMMUT_REMOTE macro to use "
                             "remote capabilities.");
#endif
}

void CpuFreq::release(CpuFreq* cpufreq){
    if(cpufreq){
        delete cpufreq;
    }
}

Domain* CpuFreq::getDomain(const topology::VirtualCore* virtualCore) const{
    std::vector<Domain*> domains = getDomains();
    for(size_t i = 0; i < domains.size(); i++){
        cpufreq::Domain* currentDomain = domains.at(i);
        if(currentDomain->contains(virtualCore)){
            return currentDomain;
        }
    }
    throw std::runtime_error("getDomain: no domain found for virtual core: " + utils::intToString(virtualCore->getVirtualCoreId()));
}

std::vector<Domain*> CpuFreq::getDomains(const std::vector<topology::VirtualCore*>& virtualCores) const{
    std::vector<Domain*> r;
    for(size_t i = 0; i < virtualCores.size(); i++){
        Domain* d = getDomain(virtualCores.at(i));
        if(!utils::contains(r, d)){
            r.push_back(d);
        }
    }
    return r;
}

std::vector<Domain*> CpuFreq::getDomains(const topology::PhysicalCore* physicalCore) const{
    return getDomains(physicalCore->getVirtualCores());
}

std::vector<Domain*> CpuFreq::getDomains(const topology::Cpu* cpu) const{
    return getDomains(cpu->getVirtualCores());
}

std::vector<Domain*> CpuFreq::getDomainsComplete(const std::vector<topology::VirtualCore*>& virtualCores) const{
    std::vector<Domain*> r;
    std::vector<Domain*> domains = getDomains();
    for(size_t i = 0; i < domains.size(); i++){
        uint contained = 0;
        for(size_t j = 0; j < virtualCores.size(); j++){
            if(domains.at(i)->contains(virtualCores.at(j))){
                ++contained;
            }
        }

        if(contained == domains.at(i)->getVirtualCores().size()){
            r.push_back(domains.at(i));
        }
    }
    return r;
}

std::vector<RollbackPoint> CpuFreq::getRollbackPoints() const{
    std::vector<RollbackPoint> r;
    std::vector<Domain*> domains = getDomains();
    for(size_t i = 0; i < domains.size(); i++){
        r.push_back(domains.at(i)->getRollbackPoint());
    }
    return r;
}

void CpuFreq::rollback(const std::vector<RollbackPoint>& rollbackPoints) const{
    std::vector<Domain*> domains = getDomains();
    for(size_t i = 0; i < rollbackPoints.size(); i++){
        RollbackPoint rp = rollbackPoints.at(i);
        domains.at(rp.domainId)->rollback(rp);
    }
}

bool CpuFreq::isGovernorAvailable(Governor governor) const{
    std::vector<Domain*> domains = getDomains();
    if(!domains.size()){
        return false;
    }
    for(size_t i = 0; i < domains.size(); i++){
        if(!domains.at(i)->isGovernorAvailable(governor)){
            return false;
        }
    }
    return true;
}

void loadVoltageTable(VoltageTable& voltageTable, std::string fileName){
    std::ifstream file;
    file.open(fileName.c_str());
    if(!file){
        throw std::runtime_error("Impossible to open the specified voltage table file.");
    }
    voltageTable.clear();

    std::string line;
    std::vector<std::string> fields;
    VoltageTableKey key;
    while(std::getline(file, line)){
        /** Skips lines starting with #. **/
        if(line.at(0) == '#'){
            continue;
        }
        fields = utils::split(line, ';');
        key.first = utils::stringToInt(fields.at(0));
        key.second = utils::stringToInt(fields.at(1));
        voltageTable.insert(std::pair<VoltageTableKey, Voltage>(key, utils::stringToDouble(fields.at(2))));
    }

}

void dumpVoltageTable(const VoltageTable& voltageTable, std::string fileName){
    std::ofstream file;
    file.open(fileName.c_str());
    if(!file){
        throw std::runtime_error("Impossible to open the specified voltage table file.");
    }

    file << "# This file contains the voltage table in the following format: " << std::endl;
    file << "# NumVirtualCores;Frequency;Voltage" << std::endl;

    for(VoltageTableIterator iterator = voltageTable.begin(); iterator != voltageTable.end(); iterator++){
        file << iterator->first.first << ";" << iterator->first.second << ";" << iterator->second << std::endl;
    }
    file.close();
}

#ifdef MAMMUT_REMOTE
std::string CpuFreq::getModuleName(){
    // Any message defined in the .proto file is ok.
    GetAvailableFrequencies gaf;
    return utils::getModuleNameFromMessage(&gaf);
}

bool CpuFreq::processMessage(const std::string& messageIdIn, const std::string& messageIn,
                             std::string& messageIdOut, std::string& messageOut){
    std::vector<Domain*> domains = getDomains();

    {
        RemoveTurboFrequencies rtf;
        if(utils::getDataFromMessage<EnableBoosting>(RemoveTurboFrequencies, messageIn, rtf)){
            ResultVoid r;
            removeTurboFrequencies();
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        IsBoostingSupported ibs;
        if(utils::getDataFromMessage<IsBoostingSupported>(messageIdIn, messageIn, ibs)){
            Result r;
            r.set_result(isBoostingSupported());
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        IsBoostingEnabled ibe;
        if(utils::getDataFromMessage<IsBoostingEnabled>(messageIdIn, messageIn, ibe)){
            Result r;
            r.set_result(isBoostingEnabled());
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        EnableBoosting eb;
        if(utils::getDataFromMessage<EnableBoosting>(messageIdIn, messageIn, eb)){
            ResultVoid r;
            enableBoosting();
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        DisableBoosting db;
        if(utils::getDataFromMessage<DisableBoosting>(messageIdIn, messageIn, db)){
            ResultVoid r;
            disableBoosting();
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetDomains gd;
        if(utils::getDataFromMessage<GetDomains>(messageIdIn, messageIn, gd)){
            GetDomainsRes r;
            DomainId domainId;
            for(unsigned int i = 0; i < domains.size(); i++){
                domainId = domains.at(i)->getId();
                utils::vectorToPbRepeated<uint32_t>(domains.at(i)->getVirtualCoresIdentifiers(),
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
            std::vector<Frequency> availableFrequencies = domains.at((gaf.id()))->getAvailableFrequencies();
            utils::vectorToPbRepeated<uint32_t>(availableFrequencies, r.mutable_frequencies());
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetAvailableGovernors gag;
        if(utils::getDataFromMessage<GetAvailableGovernors>(messageIdIn, messageIn, gag)){
            GetAvailableGovernorsRes r;
            std::vector<Governor> availableGovernors = domains.at((gag.id()))->getAvailableGovernors();
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
                r.set_frequency(domains.at((gcf.id()))->getCurrentFrequencyUserspace());
            }else{
                r.set_frequency(domains.at((gcf.id()))->getCurrentFrequency());
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetCurrentGovernor gcg;
        if(utils::getDataFromMessage<GetCurrentGovernor>(messageIdIn, messageIn, gcg)){
            GetCurrentGovernorRes r;
            r.set_governor(domains.at((gcg.id()))->getCurrentGovernor());
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        ChangeFrequency cf;
        if(utils::getDataFromMessage<ChangeFrequency>(messageIdIn, messageIn, cf)){
            Result r;
            r.set_result(domains.at((cf.id()))->setFrequencyUserspace(cf.frequency()));
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetHardwareFrequencyBounds ghfb;
        if(utils::getDataFromMessage<GetHardwareFrequencyBounds>(messageIdIn, messageIn, ghfb)){
            GetHardwareFrequencyBoundsRes r;
            Frequency lb, ub;
            domains.at((ghfb.id()))->getHardwareFrequencyBounds(lb, ub);
            r.set_lower_bound(lb);
            r.set_upper_bound(ub);
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetGovernorBounds ggb;
        if(utils::getDataFromMessage<GetGovernorBounds>(messageIdIn, messageIn, ggb)){
            GetGovernorBoundsRes r;
            Frequency lb, ub;
            bool result;
            result = domains.at((ggb.id()))->getCurrentGovernorBounds(lb, ub);
            r.set_lower_bound(lb);
            r.set_upper_bound(ub);
            r.set_result(result);
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        ChangeFrequencyBounds cfb;
        if(utils::getDataFromMessage<ChangeFrequencyBounds>(messageIdIn, messageIn, cfb)){
            Result r;
            r.set_result(domains.at((cfb.id()))->setGovernorBounds(cfb.lower_bound(), cfb.upper_bound()));
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        ChangeGovernor cg;
        if(utils::getDataFromMessage<ChangeGovernor>(messageIdIn, messageIn, cg)){
            Result r;
            r.set_result(domains.at((cg.id()))->setGovernor((Governor) cg.governor()));
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetTransitionLatency gtl;
        if(utils::getDataFromMessage<GetTransitionLatency>(messageIdIn, messageIn, gtl)){
            ResultInt r;
            r.set_result(domains.at((gtl.id()))->getTransitionLatency());
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetCurrentVoltage gcv;
        if(utils::getDataFromMessage<GetCurrentVoltage>(messageIdIn, messageIn, gcv)){
            ResultDouble r;
            r.set_result(domains.at((gcv.id()))->getCurrentVoltage());
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    return false;
}
#endif

}
}
