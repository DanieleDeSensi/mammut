/*
 * topology-linux.cpp
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

#include <mammut/utils.hpp>
#include <mammut/topology/topology-linux.hpp>

#include <fstream>
#include <stdexcept>

namespace mammut{
namespace topology{

CpuLinux::CpuLinux(CpuId cpuId, std::vector<PhysicalCore*> physicalCores):
    Cpu(cpuId, physicalCores){
    ;
}

std::string CpuLinux::getCpuInfo(const std::string& infoName) const{
    std::ifstream infile("/proc/cpuinfo");
    if(!infile.is_open()){
        throw std::runtime_error("Impossble to open /proc/cpuinfo.");
    }

    //TODO: Non una linea qualsiasi ma quella di questa cpu
    std::string line;
    while(std::getline(infile, line)){
        if(!line.compare(0, infoName.length(), infoName)){
            return utils::ltrim(utils::split(line, ':').at(1));
        }
    }
    return "";
}

std::string CpuLinux::getVendorId() const{
    return getCpuInfo("vendor_id");
}

std::string CpuLinux::getFamily() const{
    return getCpuInfo("cpu family");
}

std::string CpuLinux::getModel() const{
    return getCpuInfo("model");
}

PhysicalCoreLinux::PhysicalCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId, std::vector<VirtualCore*> virtualCores):
    PhysicalCore(cpuId, physicalCoreId, virtualCores){
    ;
}

VirtualCoreLinux::VirtualCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId, VirtualCoreId virtualCoreId):
    VirtualCore(cpuId, physicalCoreId, virtualCoreId){
    ;
}

}
}
