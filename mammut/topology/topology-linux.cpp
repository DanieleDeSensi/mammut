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

std::string getTopologyPathFromVirtualCoreId(VirtualCoreId id){
    return "/sys/devices/system/cpu/cpu" + utils::intToString(id) + "/topology/";
}

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

VirtualCoreIdleLevelLinux::VirtualCoreIdleLevelLinux(VirtualCoreId virtualCoreId, uint levelId):
    VirtualCoreIdleLevel(virtualCoreId, levelId), _path("/sys/devices/system/cpu/cpu" + utils::intToString(virtualCoreId) +
                                                        "/cpuidle/state" + utils::intToString(levelId) + "/"){
    ;
}

std::string VirtualCoreIdleLevelLinux::getName() const{
    return utils::readFirstLineFromFile(_path + "name");
}

std::string VirtualCoreIdleLevelLinux::getDesc() const{
    return utils::readFirstLineFromFile(_path + "desc");
}

bool VirtualCoreIdleLevelLinux::isEnabled() const{
    return (utils::readFirstLineFromFile(_path + "disable").compare("0") == 0);
}

void VirtualCoreIdleLevelLinux::enable() const{
    utils::executeCommand("echo 0 > " + _path + "disable");
}

void VirtualCoreIdleLevelLinux::disable() const{
    utils::executeCommand("echo 1 > " + _path + "disable");
}

uint VirtualCoreIdleLevelLinux::getExitLatency() const{
    return utils::stringToInt(utils::readFirstLineFromFile(_path + "latency"));
}

uint VirtualCoreIdleLevelLinux::getConsumedPower() const{
    return utils::stringToInt(utils::readFirstLineFromFile(_path + "power"));
}

uint VirtualCoreIdleLevelLinux::getTime() const{
    return utils::stringToInt(utils::readFirstLineFromFile(_path + "time"));
}

uint VirtualCoreIdleLevelLinux::getCount() const{
    return utils::stringToInt(utils::readFirstLineFromFile(_path + "usage"));
}

VirtualCoreLinux::VirtualCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId, VirtualCoreId virtualCoreId):
            VirtualCore(cpuId, physicalCoreId, virtualCoreId),
            _hotplugFile("/sys/devices/system/cpu/cpu" + utils::intToString(virtualCoreId) + "/online"){
    std::vector<std::string> levelsNames =
            utils::getFilesNamesInDir("/sys/devices/system/cpu/cpu" + utils::intToString(getVirtualCoreId()) + "/cpuidle", false, true);
    for(size_t i = 0; i < levelsNames.size(); i++){
        std::string levelName = levelsNames.at(i);
        if(levelName.compare(0, 5, "state") == 0){
            uint levelId = utils::stringToInt(levelName.substr(5));
            _idleLevels.push_back(new VirtualCoreIdleLevelLinux(getVirtualCoreId(), levelId));
        }
    }
}

bool VirtualCoreLinux::isHotPluggable() const{
    return utils::existsFile(_hotplugFile);
}

bool VirtualCoreLinux::isHotPlugged() const{
    std::string online = utils::readFirstLineFromFile(_hotplugFile);
    return (utils::stringToInt(online) > 0);
}

void VirtualCoreLinux::hotPlug() const{
    utils::executeCommand("echo 1 > " + _hotplugFile);
}

void VirtualCoreLinux::hotUnplug() const{
    utils::executeCommand("echo 0 > " + _hotplugFile);
}

std::vector<VirtualCoreIdleLevel*> VirtualCoreLinux::getIdleLevels() const{
    return _idleLevels;
}

VirtualCoreLinux::~VirtualCoreLinux(){
    utils::deleteVectorElements<VirtualCoreIdleLevel*>(_idleLevels);
}

}
}
