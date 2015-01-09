/*
 * topology-remote.cpp
 *
 * Created on: 14/11/2014
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

#include <mammut/topology/topology-remote.hpp>
#include <mammut/topology/topology-remote.pb.h>

#include <stdexcept>

namespace mammut{
namespace topology{

CpuRemote::CpuRemote(Communicator* const communicator, CpuId cpuId, std::vector<PhysicalCore*> physicalCores)
    :Cpu(cpuId, physicalCores), _communicator(communicator){
    ;
}

std::string CpuRemote::getVendorId() const{
    GetCpuVendorId gcvi;
    GetCpuVendorIdRes r;
    gcvi.set_cpu_id(getCpuId());
    _communicator->remoteCall(gcvi, r);
    return r.vendor_id();
}

std::string CpuRemote::getFamily() const{
    GetCpuFamily gcf;
    GetCpuFamilyRes r;
    gcf.set_cpu_id(getCpuId());
    _communicator->remoteCall(gcf, r);
    return r.family();
}

std::string CpuRemote::getModel() const{
    GetCpuModel gcm;
    GetCpuModelRes r;
    gcm.set_cpu_id(getCpuId());
    _communicator->remoteCall(gcm, r);
    return r.model();
}

PhysicalCoreRemote::PhysicalCoreRemote(Communicator* const communicator, CpuId cpuId, PhysicalCoreId physicalCoreId,
                                       std::vector<VirtualCore*> virtualCores)
    :PhysicalCore(cpuId, physicalCoreId, virtualCores), _communicator(communicator){
    ;
}

VirtualCoreIdleLevelRemote::VirtualCoreIdleLevelRemote(VirtualCoreId virtualCoreId, uint levelId, Communicator* const communicator):
    VirtualCoreIdleLevel(virtualCoreId, levelId), _communicator(communicator){
}

std::string VirtualCoreIdleLevelRemote::getName() const{
    IdleLevelGetName ilgn;
    IdleLevelGetNameRes r;
    ilgn.set_virtual_core_id(getVirtualCoreId());
    ilgn.set_level_id(getLevelId());
    _communicator->remoteCall(ilgn, r);
    return r.name();
}

std::string VirtualCoreIdleLevelRemote::getDesc() const{
    IdleLevelGetDesc ilgd;
    IdleLevelGetDescRes r;
    ilgd.set_virtual_core_id(getVirtualCoreId());
    ilgd.set_level_id(getLevelId());
    _communicator->remoteCall(ilgd, r);
    return r.description();
}

bool VirtualCoreIdleLevelRemote::isEnabled() const{
    IdleLevelIsEnabled ilie;
    IdleLevelIsEnabledRes r;
    ilie.set_virtual_core_id(getVirtualCoreId());
    ilie.set_level_id(getLevelId());
    _communicator->remoteCall(ilie, r);
    return r.enabled();
}

void VirtualCoreIdleLevelRemote::enable() const{
    IdleLevelEnable ile;
    GenericRes r;
    ile.set_virtual_core_id(getVirtualCoreId());
    ile.set_level_id(getLevelId());
    _communicator->remoteCall(ile, r);
}

void VirtualCoreIdleLevelRemote::disable() const{
    IdleLevelDisable ild;
    GenericRes r;
    ild.set_virtual_core_id(getVirtualCoreId());
    ild.set_level_id(getLevelId());
    _communicator->remoteCall(ild, r);
}

uint VirtualCoreIdleLevelRemote::getExitLatency() const{
    IdleLevelGetExitLatency ilgel;
    IdleLevelGetExitLatencyRes r;
    ilgel.set_virtual_core_id(getVirtualCoreId());
    ilgel.set_level_id(getLevelId());
    _communicator->remoteCall(ilgel, r);
    return r.exit_latency();
}

uint VirtualCoreIdleLevelRemote::getConsumedPower() const{
    IdleLevelGetConsumedPower ilgcp;
    IdleLevelGetConsumedPowerRes r;
    ilgcp.set_virtual_core_id(getVirtualCoreId());
    ilgcp.set_level_id(getLevelId());
    _communicator->remoteCall(ilgcp, r);
    return r.consumed_power();
}

uint VirtualCoreIdleLevelRemote::getTime() const{
    IdleLevelGetTime ilgt;
    IdleLevelGetTimeRes r;
    ilgt.set_virtual_core_id(getVirtualCoreId());
    ilgt.set_level_id(getLevelId());
    _communicator->remoteCall(ilgt, r);
    return r.time();
}

uint VirtualCoreIdleLevelRemote::getCount() const{
    IdleLevelGetCount ilgc;
    IdleLevelGetCountRes r;
    ilgc.set_virtual_core_id(getVirtualCoreId());
    ilgc.set_level_id(getLevelId());
    _communicator->remoteCall(ilgc, r);
    return r.count();
}

VirtualCoreRemote::VirtualCoreRemote(Communicator* const communicator, CpuId cpuId, PhysicalCoreId physicalCoreId,
                                     VirtualCoreId virtualCoreId)
    :VirtualCore(cpuId, physicalCoreId, virtualCoreId), _communicator(communicator){
    IdleLevelsGet ilg;
    IdleLevelsGetRes r;
    ilg.set_virtual_core_id(getVirtualCoreId());
    _communicator->remoteCall(ilg, r);
    for(int i = 0; i < r.level_id_size(); i++){
        _idleLevels.push_back(new VirtualCoreIdleLevelRemote(getVirtualCoreId(), r.level_id(i), _communicator));
    }
}

bool VirtualCoreRemote::isHotPluggable() const{
    IsHotPluggable ihp;
    IsHotPluggableRes r;
    ihp.set_virtual_core_id(getVirtualCoreId());
    _communicator->remoteCall(ihp, r);
    return r.result();
}

bool VirtualCoreRemote::isHotPlugged() const{
    IsHotPlugged ihp;
    IsHotPluggedRes r;
    ihp.set_virtual_core_id(getVirtualCoreId());
    _communicator->remoteCall(ihp, r);
    return r.result();
}

void VirtualCoreRemote::hotPlug() const{
    HotPlug hp;
    GenericRes r;
    hp.set_virtual_core_id(getVirtualCoreId());
    _communicator->remoteCall(hp, r);
}

void VirtualCoreRemote::hotUnplug() const{
    HotUnplug hu;
    GenericRes r;
    hu.set_virtual_core_id(getVirtualCoreId());
    _communicator->remoteCall(hu, r);
}

std::vector<VirtualCoreIdleLevel*> VirtualCoreRemote::getIdleLevels() const{
    return _idleLevels;
}

VirtualCoreRemote::~VirtualCoreRemote(){
    utils::deleteVectorElements<VirtualCoreIdleLevel*>(_idleLevels);
}

}
}
