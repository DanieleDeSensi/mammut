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
    return getCpuInfo("cpu family"); //TODO: Get with cpuid
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
    resetTime();
    resetCount();
}

std::string VirtualCoreIdleLevelLinux::getName() const{
    return utils::readFirstLineFromFile(_path + "name");
}

std::string VirtualCoreIdleLevelLinux::getDesc() const{
    return utils::readFirstLineFromFile(_path + "desc");
}

bool VirtualCoreIdleLevelLinux::isEnableable() const{
    return utils::existsFile(_path + "disable");
}

bool VirtualCoreIdleLevelLinux::isEnabled() const{
    if(isEnableable()){
        return (utils::readFirstLineFromFile(_path + "disable").compare("0") == 0);
    }else{
        return true;
    }
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

uint VirtualCoreIdleLevelLinux::getAbsoluteTime() const{
    return utils::stringToInt(utils::readFirstLineFromFile(_path + "time"));
}

uint VirtualCoreIdleLevelLinux::getTime() const{
    return getAbsoluteTime() - _lastAbsTime;
}

void VirtualCoreIdleLevelLinux::resetTime(){
    _lastAbsTime = getAbsoluteTime();
}

uint VirtualCoreIdleLevelLinux::getAbsoluteCount() const{
    return utils::stringToInt(utils::readFirstLineFromFile(_path + "usage"));
}

uint VirtualCoreIdleLevelLinux::getCount() const{
    return getAbsoluteCount() - _lastAbsCount;
}

void VirtualCoreIdleLevelLinux::resetCount(){
    _lastAbsCount = getAbsoluteCount();
}

VirtualCoreLinux::VirtualCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId, VirtualCoreId virtualCoreId):
            VirtualCore(cpuId, physicalCoreId, virtualCoreId),
            _hotplugFile("/sys/devices/system/cpu/cpu" + utils::intToString(virtualCoreId) + "/online"),
            _msr(virtualCoreId){
    std::vector<std::string> levelsNames =
            utils::getFilesNamesInDir("/sys/devices/system/cpu/cpu" + utils::intToString(getVirtualCoreId()) + "/cpuidle", false, true);
    for(size_t i = 0; i < levelsNames.size(); i++){
        std::string levelName = levelsNames.at(i);
        if(levelName.compare(0, 5, "state") == 0){
            uint levelId = utils::stringToInt(levelName.substr(5));
            _idleLevels.push_back(new VirtualCoreIdleLevelLinux(getVirtualCoreId(), levelId));
        }
    }
    resetIdleTime();
}

#define MSR_PERF_STATUS 0x198
double VirtualCoreLinux::getCurrentVoltage() const{
    if(_msr.available()){
        return (float)_msr.readBits(MSR_PERF_STATUS, 47, 32) / (float)(1 << 13);
    }else{
        return 0;
    }
}

uint VirtualCoreLinux::getIdleTime() const{
    if(_idleLevels.size() && 0){
        uint r = 0;
        for(size_t i = 0; i < _idleLevels.size(); i++){
            r += (_idleLevels.at(i)->getAbsoluteTime());
        }
        return r - _lastProcIdleTime;
    }else{
        std::string tmp = mammut::utils::getCommandOutput("cat /proc/stat | grep cpu" + utils::intToString(getVirtualCoreId()) + " | cut -d ' ' -f 5").at(0);
        return (((double)mammut::utils::stringToInt(tmp)/(double)utils::getClockTicksPerSecond()) - _lastProcIdleTime) * 1000000.0;
    }
}

void VirtualCoreLinux::resetIdleTime(){
    if(_idleLevels.size() && 0){
        _lastProcIdleTime = 0;
        for(size_t i = 0; i < _idleLevels.size(); i++){
            _lastProcIdleTime += _idleLevels.at(i)->getAbsoluteTime();
        }
    }else{
        std::string tmp = mammut::utils::getCommandOutput("cat /proc/stat | grep cpu" + utils::intToString(getVirtualCoreId()) + " | cut -d ' ' -f 5").at(0);
        _lastProcIdleTime = (double)mammut::utils::stringToInt(tmp)/(double)utils::getClockTicksPerSecond();
    }
}

bool VirtualCoreLinux::isHotPluggable() const{
    return utils::existsFile(_hotplugFile);
}

bool VirtualCoreLinux::isHotPlugged() const{
    if(isHotPluggable()){
        std::string online = utils::readFirstLineFromFile(_hotplugFile);
        return (utils::stringToInt(online) > 0);
    }else{
        return true;
    }
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
