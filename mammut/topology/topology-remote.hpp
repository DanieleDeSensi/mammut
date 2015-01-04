/*
 * topology-remote.hpp
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

#ifndef TOPOLOGY_REMOTE_HPP_
#define TOPOLOGY_REMOTE_HPP_

#include <mammut/topology/topology.hpp>

namespace mammut{
namespace topology{

class CpuRemote: public Cpu{
private:
    Communicator* const _communicator;
public:
    CpuRemote(Communicator* const communicator, CpuId cpuId, std::vector<PhysicalCore*> physicalCores);
    std::string getVendorId() const;
    std::string getFamily() const;
    std::string getModel() const;
};

class PhysicalCoreRemote: public PhysicalCore{
private:
    Communicator* const _communicator;
public:
    PhysicalCoreRemote(Communicator* const communicator, CpuId cpuId, PhysicalCoreId physicalCoreId,
                       std::vector<VirtualCore*> virtualCores);
};

class VirtualCoreRemote: public VirtualCore{
private:
    Communicator* const _communicator;
public:
    VirtualCoreRemote(Communicator* const communicator, CpuId cpuId, PhysicalCoreId physicalCoreId,
                      VirtualCoreId virtualCoreId);
    bool isHotPluggable() const;
    bool isHotPlugged() const;
    void hotPlug() const;
    void hotUnplug() const;
};

}
}


#endif /* TOPOLOGY_REMOTE_HPP_ */
