/*
 * topology.cpp
 *
 * Created on: 20/11/2014
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

#include <mammut/topology/topology.hpp>
#include <mammut/topology/topology-linux.hpp>
#include <mammut/topology/topology-remote.hpp>
#include <mammut/topology/topology-remote.pb.h>
#include <mammut/utils.hpp>

#include <stddef.h>
#include <stdexcept>
#include <string>

namespace mammut{
namespace topology{

Topology::Topology():_communicator(NULL){
#if defined(__linux__)
    std::vector<VirtualCoreCoordinates> coord;
    std::string range;
    std::string path;
    int lowestCoreId, highestCoreId;
    std::map<std::pair<CpuId, PhysicalCoreId>, PhysicalCoreId> uniquePhysicalCoreIds;
    PhysicalCoreId nextIdToUse = 0;

    const std::string coresListFile = "/sys/devices/system/cpu/present";
    if(utils::existsFile(coresListFile)){
        range = utils::readFirstLineFromFile(coresListFile);
        utils::dashedRangeToIntegers(range, lowestCoreId, highestCoreId);
    }else{
        std::vector coresIdentifiers = utils::getCommandOutput("ls /sys/devices/system/cpu/ | "
                                                              "grep cpu | "
                                                              "sed  's/cpu//g' | "
                                                              "sort -n");
        lowestCoreId = utils::stringToInt(coresIdentifiers.front());
        highestCoreId = utils::stringToInt(coresIdentifiers.back());
    }


    for(unsigned int virtualCoreId = (uint) lowestCoreId; virtualCoreId <= (uint) highestCoreId; virtualCoreId++){
        path = getTopologyPathFromVirtualCoreId(virtualCoreId);
        VirtualCoreCoordinates vcc;
        vcc.cpuId = utils::stringToInt(utils::readFirstLineFromFile(path + "physical_package_id"));
        vcc.physicalCoreId = utils::stringToInt(utils::readFirstLineFromFile(path + "core_id"));
        vcc.virtualCoreId = virtualCoreId;

        /**
         * core_id is not unique. The pair <physical_package_id, core_id> is unique.
         * Accordingly, we modify physicalCoreId to let it unique.
         */
        std::map<std::pair<CpuId, PhysicalCoreId>, PhysicalCoreId>::iterator it;
        std::pair<CpuId, PhysicalCoreId> identifiersPair(vcc.cpuId, vcc.physicalCoreId);
        it = uniquePhysicalCoreIds.find(identifiersPair);
        if(it == uniquePhysicalCoreIds.end()){
            uniquePhysicalCoreIds.insert(std::pair<std::pair<CpuId, PhysicalCoreId>, PhysicalCoreId>(identifiersPair, nextIdToUse));
            vcc.physicalCoreId = nextIdToUse;
            ++nextIdToUse;
        }else{
            vcc.physicalCoreId = it->second;
        }

        coord.push_back(vcc);
    }

    buildCpuVector(coord);
#else
    throw std::exception("Topology: OS not supported.");
#endif
}

Topology* Topology::local(){
#if defined(__linux__)
    return new TopologyLinux();
#else
    throw std::exception("Topology: OS not supported.");
#endif
}

Topology::Topology(Communicator* const communicator):_communicator(communicator){
    GetTopology gt;
    GetTopologyRes r;

    _communicator->remoteCall(gt, r);
    std::vector<VirtualCoreCoordinates> coord;
    VirtualCoreCoordinates vcc;

    for(size_t i = 0; i < (size_t) r.coordinates().size(); i++){
        vcc.cpuId = r.coordinates(i).cpu_id();
        vcc.physicalCoreId = r.coordinates(i).physical_core_id();
        vcc.virtualCoreId = r.coordinates(i).virtual_core_id();
        coord.push_back(vcc);
    }

    buildCpuVector(coord);
}

Topology* Topology::remote(Communicator* const communicator){
    return new TopologyRemote(communicator);
}


Topology::~Topology(){
    utils::deleteVectorElements<Cpu*>(_cpus);
    utils::deleteVectorElements<PhysicalCore*>(_physicalCores);
    utils::deleteVectorElements<VirtualCore*>(_virtualCores);
}

void Topology::release(Topology* topology){
    if(topology){
        delete topology;
    }
}

std::vector<Cpu*> Topology::getCpus() const{
    return _cpus;
}

std::vector<PhysicalCore*> Topology::getPhysicalCores() const{
    return _physicalCores;
}

std::vector<VirtualCore*> Topology::getVirtualCores() const{
    return _virtualCores;
}

std::vector<PhysicalCore*> Topology::virtualToPhysical(const std::vector<VirtualCore*>& virtualCores) const{
    bool contained;
    std::vector<topology::PhysicalCore*> physicalCores;
    for(size_t i = 0; i < virtualCores.size(); i++){
        topology::PhysicalCore* p = getPhysicalCore(virtualCores.at(i)->getPhysicalCoreId());
        contained = false;
        for(size_t j = 0; j < physicalCores.size(); j++){
            if(*(physicalCores.at(j)) ==(*p)){
                contained = true;
                break;
            }
        }
        if(!contained){
            physicalCores.push_back(p);
        }
        contained = false;
    }
    return physicalCores;
}

Cpu* Topology::getCpu(CpuId cpuId) const{
    Cpu* c = NULL;
    for(size_t i = 0; i < _cpus.size(); i++){
        c = _cpus.at(i);
        if(c->getCpuId() == cpuId){
            return c;
        }
    }
    return NULL;
}

PhysicalCore* Topology::getPhysicalCore(PhysicalCoreId physicalCoreId) const{
    PhysicalCore* pc = NULL;
    for(size_t i = 0; i < _cpus.size(); i++){
        pc = _cpus.at(i)->getPhysicalCore(physicalCoreId);
        if(pc){
            return pc;
        }
    }
    return NULL;
}

VirtualCore* Topology::getVirtualCore(VirtualCoreId virtualCoreId) const{
    VirtualCore* vc = NULL;
    for(size_t i = 0; i < _cpus.size(); i++){
        vc = _cpus.at(i)->getVirtualCore(virtualCoreId);
        if(vc){
            return vc;
        }
    }
    return NULL;
}

void Topology::buildCpuVector(std::vector<VirtualCoreCoordinates> coord){
    std::vector<CpuId> uniqueCpuIds;

    for(size_t i = 0; i < coord.size(); i++){
        if(!utils::contains<CpuId>(uniqueCpuIds, coord.at(i).cpuId)){
            uniqueCpuIds.push_back(coord.at(i).cpuId);
        }
    }

    _cpus.reserve(uniqueCpuIds.size());
    for(size_t i = 0; i < uniqueCpuIds.size(); i++){
        Cpu* c = NULL;
        std::vector<PhysicalCore*> phy = buildPhysicalCoresVector(coord, uniqueCpuIds.at(i));
        if(_communicator){
            c = new CpuRemote(_communicator, uniqueCpuIds.at(i), phy);
        }else{
#if defined (__linux__)
            c = new CpuLinux(uniqueCpuIds.at(i), phy);
#else
            throw std::runtime_error("buildCpuVector: OS not supported");
#endif
        }
        _cpus.push_back(c);
    }
}

std::vector<PhysicalCore*> Topology::buildPhysicalCoresVector(std::vector<VirtualCoreCoordinates> coord, CpuId cpuId){
    std::vector<PhysicalCore*> physicalCores;
    std::vector<PhysicalCoreId> usedIdentifiers;
    for(size_t i = 0; i < coord.size(); i++){
        VirtualCoreCoordinates vcc = coord.at(i);
        if(vcc.cpuId == cpuId && !utils::contains<PhysicalCoreId>(usedIdentifiers, vcc.physicalCoreId)){
            PhysicalCore* p =  NULL;
            std::vector<VirtualCore*> vir = buildVirtualCoresVector(coord, vcc.cpuId, vcc.physicalCoreId);
            if(_communicator){
                p = new PhysicalCoreRemote(_communicator, vcc.cpuId, vcc.physicalCoreId, vir);
            }else{
#if defined (__linux__)
                p = new PhysicalCoreLinux(vcc.cpuId, vcc.physicalCoreId, vir);
#else
                throw std::runtime_error("buildPhysicalCoresVector: OS not supported");
#endif
            }
            physicalCores.push_back(p);
            _physicalCores.push_back(p);
            usedIdentifiers.push_back(vcc.physicalCoreId);
        }
    }
    return physicalCores;
}

std::vector<VirtualCore*> Topology::buildVirtualCoresVector(std::vector<VirtualCoreCoordinates> coord, CpuId cpuId, PhysicalCoreId physicalCoreId){
    std::vector<VirtualCore*> virtualCores;
    for(size_t i = 0; i < coord.size(); i++){
        VirtualCoreCoordinates vcc = coord.at(i);
        if(vcc.cpuId == cpuId && vcc.physicalCoreId == physicalCoreId){
            VirtualCore* v = NULL;
            if(_communicator){
                v = new VirtualCoreRemote(_communicator, vcc.cpuId, vcc.physicalCoreId, vcc.virtualCoreId);;
            }else{
#if defined (__linux__)
                v = new VirtualCoreLinux(vcc.cpuId, vcc.physicalCoreId, vcc.virtualCoreId);;
#else
                throw std::runtime_error("buildVirtualCoresVector: OS not supported");
#endif
            }
            virtualCores.push_back(v);
            _virtualCores.push_back(v);
        }
    }
    return virtualCores;
}

VirtualCore* Topology::getVirtualCore() const{
    if(_virtualCores.size()){
        return _virtualCores.at(0);
    }else{
        return NULL;
    }
}

Cpu::Cpu(CpuId cpuId, std::vector<PhysicalCore*> physicalCores):_cpuId(cpuId),
                                                                _physicalCores(physicalCores),
                                                                _virtualCores(virtualCoresFromPhysicalCores()){
    ;
}

std::vector<VirtualCore*> Cpu::virtualCoresFromPhysicalCores(){
    std::vector<VirtualCore*> v;
    for(size_t i = 0; i < _physicalCores.size(); i++){
        std::vector<VirtualCore*> t = _physicalCores.at(i)->getVirtualCores();
        v.insert(v.end(), t.begin(), t.end());
    }
    return v;
}

CpuId Cpu::getCpuId() const{
    return _cpuId;
}

std::vector<PhysicalCore*> Cpu::getPhysicalCores() const{
    return _physicalCores;
}

std::vector<VirtualCore*> Cpu::getVirtualCores() const{
    return _virtualCores;
}

PhysicalCore* Cpu::getPhysicalCore(PhysicalCoreId physicalCoreId) const{
    PhysicalCore* pc = NULL;
    for(size_t i = 0; i < _physicalCores.size(); i++){
        pc = _physicalCores.at(i);
        if(pc->getPhysicalCoreId() == physicalCoreId){
            return pc;
        }
    }
    return NULL;
}

VirtualCore* Cpu::getVirtualCore(VirtualCoreId virtualCoreId) const{
    VirtualCore* vc = NULL;
    for(size_t i = 0; i < _physicalCores.size(); i++){
        vc = _physicalCores.at(i)->getVirtualCore(virtualCoreId);
        if(vc){
            return vc;
        }
    }
    return NULL;
}

VirtualCore* Cpu::getVirtualCore() const{
    if(_virtualCores.size()){
        return _virtualCores.at(0);
    }else{
        return NULL;
    }
}

PhysicalCore::PhysicalCore(CpuId cpuId, PhysicalCoreId physicalCoreId, std::vector<VirtualCore*> virtualCores):
    _cpuId(cpuId), _physicalCoreId(physicalCoreId), _virtualCores(virtualCores){
    ;
}

PhysicalCoreId PhysicalCore::getPhysicalCoreId() const{
    return _physicalCoreId;
}

CpuId PhysicalCore::getCpuId() const{
    return _cpuId;
}

std::vector<VirtualCore*> PhysicalCore::getVirtualCores() const{
    return _virtualCores;
}

VirtualCore* PhysicalCore::getVirtualCore(VirtualCoreId virtualCoreId) const{
    VirtualCore* vc = NULL;
    for(size_t i = 0; i < _virtualCores.size(); i++){
        vc = _virtualCores.at(i);
        if(vc->getVirtualCoreId() == virtualCoreId){
            return vc;
        }
    }
    return NULL;
}

VirtualCore* PhysicalCore::getVirtualCore() const{
    if(_virtualCores.size()){
        return _virtualCores.at(0);
    }else{
        return NULL;
    }
}

VirtualCoreIdleLevel::VirtualCoreIdleLevel(VirtualCoreId virtualCoreId, uint levelId):
        _virtualCoreId(virtualCoreId), _levelId(levelId){
    ;
}

VirtualCoreId VirtualCoreIdleLevel::getVirtualCoreId() const{
    return _virtualCoreId;
}

uint VirtualCoreIdleLevel::getLevelId() const{
    return _levelId;
}

VirtualCore::VirtualCore(CpuId cpuId, PhysicalCoreId physicalCoreId, VirtualCoreId virtualCoreId):
        _cpuId(cpuId), _physicalCoreId(physicalCoreId), _virtualCoreId(virtualCoreId){
    ;
}

VirtualCoreId VirtualCore::getVirtualCoreId() const{
    return _virtualCoreId;
}

PhysicalCoreId VirtualCore::getPhysicalCoreId() const{
    return _physicalCoreId;
}

CpuId VirtualCore::getCpuId() const{
    return _cpuId;
}

size_t getNumPhysicalCores(const std::vector<VirtualCore*>& virtualCores){
    return getOneVirtualPerPhysical(virtualCores).size();
}

std::vector<VirtualCore*> getOneVirtualPerPhysical(const std::vector<VirtualCore*>& virtualCores){
    std::vector<VirtualCore*> r;
    bool found = false;
    for(size_t i = 0; i < virtualCores.size(); i++){
        VirtualCore* vc = virtualCores.at(i);
        for(size_t j = 0; j < r.size(); j++){
            if(r.at(j)->getPhysicalCoreId() == vc->getPhysicalCoreId()){
                found = true;
                break;
            }
        }

        if(!found){
            r.push_back(vc);
        }

        found = false;
    }
    return r;
}

std::string Topology::getModuleName(){
    GetTopology gt;
    return utils::getModuleNameFromMessage(&gt);
}

#define PROCESS_CPU_REQUEST(REQUEST_TYPE, RESPONSE_TYPE, PROCESSING) do{                         \
    REQUEST_TYPE req;                                                                            \
    if(utils::getDataFromMessage<REQUEST_TYPE>(messageIdIn, messageIn, req)){                    \
        RESPONSE_TYPE res;                                                                       \
        Cpu* c = getCpu(req.cpu_id());                                                           \
        if(c){                                                                                   \
            PROCESSING                                                                           \
        }else{                                                                                   \
            throw std::runtime_error("FATAL exception. Operation required on non existing CPU. " \
                                     "This should never happen.");                               \
        }                                                                                        \
        return utils::setMessageFromData(&res, messageIdOut, messageOut);                        \
    }                                                                                            \
}while(0)                                                                                        \

#define PROCESS_VIRTUAL_CORE_REQUEST(REQUEST_TYPE, RESPONSE_TYPE, PROCESSING) do{                        \
    REQUEST_TYPE req;                                                                                    \
    if(utils::getDataFromMessage<REQUEST_TYPE>(messageIdIn, messageIn, req)){                            \
        RESPONSE_TYPE res;                                                                               \
        VirtualCore* vc = getVirtualCore(req.virtual_core_id());                                         \
        if(vc){                                                                                          \
            PROCESSING                                                                                   \
        }else{                                                                                           \
            throw std::runtime_error("FATAL exception. Operation required on non existing VirtualCore. " \
                                     "This should never happen.");                                       \
        }                                                                                                \
        return utils::setMessageFromData(&res, messageIdOut, messageOut);                                \
    }                                                                                                    \
}while(0)                                                                                                \


#define PROCESS_IDLE_LEVEL(PROCESSING) do{                           \
    std::vector<VirtualCoreIdleLevel*> levels = vc->getIdleLevels(); \
    if(levels.size() == 0){                                          \
        throw std::runtime_error("Idle levels not present.");        \
    }else{                                                           \
        for(uint i = 0; i < levels.size(); i++){                     \
            VirtualCoreIdleLevel* level = levels.at(i);              \
            if(level->getLevelId() == req.level_id()){               \
                PROCESSING                                           \
            }                                                        \
        }                                                            \
    }                                                                \
}while(0);                                                           \

#define PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(REQUEST_TYPE, RESPONSE_TYPE, PROCESSING) do{       \
        PROCESS_VIRTUAL_CORE_REQUEST(REQUEST_TYPE, RESPONSE_TYPE, PROCESS_IDLE_LEVEL(PROCESSING)); \
}while(0)                                                                                          \

bool Topology::processMessage(const std::string& messageIdIn, const std::string& messageIn,
                             std::string& messageIdOut, std::string& messageOut){
    {
        GetTopology gt;
        if(utils::getDataFromMessage<GetTopology>(messageIdIn, messageIn, gt)){
            GetTopologyRes r;
            std::vector<VirtualCore*> virtualCores = getVirtualCores();
            r.mutable_coordinates()->Reserve(virtualCores.size());

            for(size_t i = 0; i < virtualCores.size(); i++){
                VirtualCore* vc = virtualCores.at(i);
                GetTopologyRes_Vcc* outCoord = r.mutable_coordinates()->Add();
                outCoord->set_cpu_id(vc->getCpuId());
                outCoord->set_physical_core_id(vc->getPhysicalCoreId());
                outCoord->set_virtual_core_id(vc->getVirtualCoreId());
            }

            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        SetUtilization su;
        if(utils::getDataFromMessage<SetUtilization>(messageIdIn, messageIn, su)){
            ResultVoid r;
            Unit* unit = NULL;
            switch(su.unit_type()){
                case SetUtilization_UnitType_TOPOLOGY:{
                    unit = this;
                }break;
                case SetUtilization_UnitType_CPU:{
                    unit = getCpu(su.id());
                }break;
                case SetUtilization_UnitType_PHYSICAL_CORE:{
                    unit = getPhysicalCore(su.id());
                }break;
                case SetUtilization_UnitType_VIRTUAL_CORE:{
                    unit = getVirtualCore(su.id());
                }break;
            }

            if(!unit){
                throw std::runtime_error("Operation required on non existing unit.");
            }

            if(su.type() == SetUtilization_Type_MAXIMIZE){
                unit->maximizeUtilization();
            }else{
                unit->resetUtilization();
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    PROCESS_CPU_REQUEST(GetCpuVendorId, GetCpuVendorIdRes, res.set_vendor_id(c->getVendorId()););
    PROCESS_CPU_REQUEST(GetCpuFamily, GetCpuFamilyRes, res.set_family(c->getFamily()););
    PROCESS_CPU_REQUEST(GetCpuModel, GetCpuModelRes, res.set_model(c->getModel()););

    PROCESS_VIRTUAL_CORE_REQUEST(GetAbsoluteTicks, ResultUint64, res.set_result(vc->getAbsoluteTicks()););
    PROCESS_VIRTUAL_CORE_REQUEST(IsHotPluggable, ResultBool, res.set_result(vc->isHotPluggable()););
    PROCESS_VIRTUAL_CORE_REQUEST(IsHotPlugged, ResultBool, res.set_result(vc->isHotPlugged()););
    PROCESS_VIRTUAL_CORE_REQUEST(HotPlug, ResultVoid, vc->hotPlug(););
    PROCESS_VIRTUAL_CORE_REQUEST(HotUnplug, ResultVoid, vc->hotUnplug(););
    PROCESS_VIRTUAL_CORE_REQUEST(GetIdleTime, GetIdleTimeRes, res.set_idle_time(vc->getIdleTime()););
    PROCESS_VIRTUAL_CORE_REQUEST(ResetIdleTime, ResultVoid, vc->resetIdleTime(););

    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelGetName, IdleLevelGetNameRes, res.set_name(level->getName()););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelGetDesc, IdleLevelGetDescRes, res.set_description(level->getDesc()););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelIsEnableable, ResultBool, res.set_result(level->isEnableable()););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelIsEnabled, ResultBool, res.set_result(level->isEnabled()););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelEnable, ResultVoid, level->enable(););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelDisable, ResultVoid, level->disable(););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelGetExitLatency, IdleLevelGetExitLatencyRes, res.set_exit_latency(level->getExitLatency()););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelGetConsumedPower, IdleLevelGetConsumedPowerRes, res.set_consumed_power(level->getConsumedPower()););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelGetAbsTime, IdleLevelGetTimeRes, res.set_time(level->getAbsoluteTime()););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelGetTime, IdleLevelGetTimeRes, res.set_time(level->getTime()););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelResetTime, ResultVoid, level->resetTime(););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelGetAbsCount, IdleLevelGetCountRes, res.set_count(level->getAbsoluteCount()););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelGetCount, IdleLevelGetCountRes, res.set_count(level->getCount()););
    PROCESS_VIRTUAL_CORE_REQUEST_IDLE_LEVEL(IdleLevelResetCount, ResultVoid, level->resetCount(););

    {
        IdleLevelsGet ilg;
        if(utils::getDataFromMessage<IdleLevelsGet>(messageIdIn, messageIn, ilg)){
            IdleLevelsGetRes res;
            VirtualCore* vc = getVirtualCore(ilg.virtual_core_id());
            std::vector<VirtualCoreIdleLevel*> levels;
            if(vc){
                levels = vc->getIdleLevels();
                for(uint i = 0; i < levels.size(); i++){
                    res.add_level_id(levels.at(i)->getLevelId());
                }
            }else{
                throw std::runtime_error("FATAL exception. Operation required on non existing VirtualCore. This should never happen.");
            }
            return utils::setMessageFromData(&res, messageIdOut, messageOut);
        }
    }

    return false;
}

}
}


