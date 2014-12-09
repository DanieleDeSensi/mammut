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
    gcvi.set_cpu_id(getId());
    _communicator->remoteCall(gcvi, r);
    return r.vendor_id();
}

std::string CpuRemote::getFamily() const{
    GetCpuFamily gcf;
    GetCpuFamilyRes r;
    gcf.set_cpu_id(getId());
    _communicator->remoteCall(gcf, r);
    return r.family();
}

std::string CpuRemote::getModel() const{
    GetCpuModel gcm;
    GetCpuModelRes r;
    gcm.set_cpu_id(getId());
    _communicator->remoteCall(gcm, r);
    return r.model();
}

PhysicalCoreRemote::PhysicalCoreRemote(Communicator* const communicator, CpuId cpuId, PhysicalCoreId physicalCoreId,
                                       std::vector<VirtualCore*> virtualCores)
    :PhysicalCore(cpuId, physicalCoreId, virtualCores){
    ;
}



VirtualCoreRemote::VirtualCoreRemote(Communicator* const communicator, CpuId cpuId, PhysicalCoreId physicalCoreId,
                                     VirtualCoreId virtualCoreId)
    :VirtualCore(cpuId, physicalCoreId, virtualCoreId){
    ;
}


}
}
