/*
 * topology-linux.hpp
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

#ifndef TOPOLOGY_LINUX_HPP_
#define TOPOLOGY_LINUX_HPP_

#include "topology.hpp"

namespace mammut{
namespace topology{

std::string getTopologyPathFromVirtualCoreId(VirtualCoreId id);

class CpuLinux: public Cpu{
private:
    std::string getCpuInfo(const std::string& infoName) const;
public:
    CpuLinux(CpuId cpuId, std::vector<PhysicalCore*> physicalCores);
    std::string getVendorId() const;
    std::string getFamily() const;
    std::string getModel() const;
};

class PhysicalCoreLinux: public PhysicalCore{
public:
    PhysicalCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId, std::vector<VirtualCore*> virtualCores);
};

class VirtualCoreIdleLevelLinux: public VirtualCoreIdleLevel{
private:
    std::string _path;
    uint _lastAbsTime;
    uint _lastAbsCount;
public:
    VirtualCoreIdleLevelLinux(VirtualCoreId virtualCoreId, uint levelId);
    std::string getName() const;
    std::string getDesc() const;
    bool isEnabled() const;
    void enable() const;
    void disable() const;
    uint getExitLatency() const;
    uint getConsumedPower() const;
    uint getAbsoluteTime() const;
    uint getTime() const;
    void resetTime();
    uint getAbsoluteCount() const;
    uint getCount() const;
    void resetCount();
};

class VirtualCoreLinux: public VirtualCore{
private:
    std::string _hotplugFile;
    std::vector<VirtualCoreIdleLevel*> _idleLevels;
    uint _lastProcIdleTime;
    utils::Msr _msr;
public:
    VirtualCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId, VirtualCoreId virtualCoreId);
    ~VirtualCoreLinux();

    double getCurrentVoltage() const;
    uint getIdleTime() const;
    void resetIdleTime();

    bool isHotPluggable() const;
    bool isHotPlugged() const;
    void hotPlug() const;
    void hotUnplug() const;

    std::vector<VirtualCoreIdleLevel*> getIdleLevels() const;
};

}
}

#endif /* TOPOLOGY_LINUX_HPP_ */
