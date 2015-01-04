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

    range = utils::readFirstLineFromFile("/sys/devices/system/cpu/present");
    utils::dashedRangeToIntegers(range, lowestCoreId, highestCoreId);

    for(unsigned int virtualCoreId = (uint) lowestCoreId; virtualCoreId <= (uint) highestCoreId; virtualCoreId++){
        path = getTopologyPathFromVirtualCoreId(virtualCoreId);
        VirtualCoreCoordinates vcc;
        vcc.cpuId = utils::stringToInt(utils::readFirstLineFromFile(path + "physical_package_id"));
        vcc.physicalCoreId = utils::stringToInt(utils::readFirstLineFromFile(path + "core_id"));
        vcc.virtualCoreId = virtualCoreId;
        coord.push_back(vcc);
    }

    buildCpuVector(coord);
#else
    throw std::exception("Topology: OS not supported.");
#endif
}

Topology* Topology::local(){
    return new Topology();
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
    return new Topology(communicator);
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

std::string Topology::getModuleName(){
    GetTopology gt;
    return utils::getModuleNameFromMessage(&gt);
}

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
        GetCpuVendorId gcvi;
        if(utils::getDataFromMessage<GetCpuVendorId>(messageIdIn, messageIn, gcvi)){
            GetCpuVendorIdRes r;
            Cpu* c = getCpu(gcvi.cpu_id());
            if(c){
                r.set_vendor_id(c->getVendorId());
            }else{
                throw std::runtime_error("FATAL exception. Operation required on non existing CPU. This should never happen.");
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetCpuFamily gcf;
        if(utils::getDataFromMessage<GetCpuFamily>(messageIdIn, messageIn, gcf)){
            GetCpuFamilyRes r;
            Cpu* c = getCpu(gcf.cpu_id());
            if(c){
                r.set_family(c->getFamily());
            }else{
                throw std::runtime_error("FATAL exception. Operation required on non existing CPU. This should never happen.");
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        GetCpuModel gcm;
        if(utils::getDataFromMessage<GetCpuModel>(messageIdIn, messageIn, gcm)){
            GetCpuModelRes r;
            Cpu* c = getCpu(gcm.cpu_id());
            if(c){
                r.set_model(c->getModel());
            }else{
                throw std::runtime_error("FATAL exception. Operation required on non existing CPU. This should never happen.");
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        IsHotPluggable ihp;
        if(utils::getDataFromMessage<IsHotPluggable>(messageIdIn, messageIn, ihp)){
            IsHotPluggableRes r;
            VirtualCore* vc = getVirtualCore(ihp.virtual_core_id());
            if(vc){
                r.set_result(vc->isHotPluggable());
            }else{
                throw std::runtime_error("FATAL exception. Operation required on non existing VirtualCore. This should never happen.");
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        IsHotPlugged ihp;
        if(utils::getDataFromMessage<IsHotPlugged>(messageIdIn, messageIn, ihp)){
            IsHotPluggedRes r;
            VirtualCore* vc = getVirtualCore(ihp.virtual_core_id());
            if(vc){
                r.set_result(vc->isHotPlugged());
            }else{
                throw std::runtime_error("FATAL exception. Operation required on non existing VirtualCore. This should never happen.");
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        HotPlug hp;
        if(utils::getDataFromMessage<HotPlug>(messageIdIn, messageIn, hp)){
            GenericRes r;
            VirtualCore* vc = getVirtualCore(hp.virtual_core_id());
            if(vc){
                vc->hotPlug();
            }else{
                throw std::runtime_error("FATAL exception. Operation required on non existing VirtualCore. This should never happen.");
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    {
        HotUnplug hu;
        if(utils::getDataFromMessage<HotUnplug>(messageIdIn, messageIn, hu)){
            GenericRes r;
            VirtualCore* vc = getVirtualCore(hu.virtual_core_id());
            if(vc){
                vc->hotUnplug();
            }else{
                throw std::runtime_error("FATAL exception. Operation required on non existing VirtualCore. This should never happen.");
            }
            return utils::setMessageFromData(&r, messageIdOut, messageOut);
        }
    }

    return false;
}

}
}


