/*
 * process-linux.cpp
 *
 * Created on: 09/01/2015
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

#include <mammut/process/process-linux.hpp>

#include <unistd.h>

namespace mammut{
namespace process{

typedef enum{
    /** Fields documentation at: http://man7.org/linux/man-pages/man5/proc.5.html. **/
    PROC_STAT_PID = 0,
    PROC_STAT_COMM,
    PROC_STAT_STATE,
    PROC_STAT_PPID,
    PROC_STAT_PGRP,
    PROC_STAT_SESSION,
    PROC_STAT_TTY_NR,
    PROC_STAT_TPGID,
    PROC_STAT_FLAGS,
    PROC_STAT_MINFLT,
    PROC_STAT_CMINFLT,
    PROC_STAT_MAJFLT,
    PROC_STAT_CMAJFLT,
    PROC_STAT_UTIME,
    PROC_STAT_STIME,
    PROC_STAT_CUTIME,
    PROC_STAT_CSTIME,
    PROC_STAT_PRIORITY,
    PROC_STAT_NICE,
    PROC_STAT_NUM_THREADS,
    PROC_STAT_ITREALVALUE,
    PROC_STAT_STARTTIME,
    PROC_STAT_VSIZE,
    PROC_STAT_RSS,
    PROC_STAT_RSSLIM,
    PROC_STAT_STARTCODE,
    PROC_STAT_ENDCODE,
    PROC_STAT_STARTSTACK,
    PROC_STAT_KSTKESP,
    PROC_STAT_KSTKEIP,
    PROC_STAT_SIGNAL,
    PROC_STAT_BLOCKED,
    PROC_STAT_SIGIGNORE,
    PROC_STAT_SIGCATCH,
    PROC_STAT_WCHAN,
    PROC_STAT_NSWAP,
    PROC_STAT_CNSWAP,
    PROC_STAT_EXIT_SIGNAL,
    PROC_STAT_PROCESSOR,
    PROC_STAT_RT_PRIORITY,
    PROC_STAT_POLICY,
    PROC_STAT_DELAYACCT_BLKIO_TICKS,
    PROC_STAT_GUEST_TIME,
    PROC_STAT_CGUEST_TIME,
    PROC_STAT_START_DATA,
    PROC_STAT_END_DATA,
    PROC_STAT_START_BRK,
    PROC_STAT_ARG_START,
    PROC_STAT_ARG_END,
    PROC_STAT_ENV_START,
    PROC_STAT_ENV_END,
    PROC_STAT_EXIT_CODE
}ProcStatFields;

ExecutionUnitLinux::ExecutionUnitLinux(Pid id, std::string path):_id(id), _path(path), _hertz(utils::getClockTicksPerSecond()){
    resetCoreUsage();
}

std::string ExecutionUnitLinux::getPath() const{
    return _path;
}

double ExecutionUnitLinux::getUpTime() const{
    std::string v;
    v = utils::split(utils::readFirstLineFromFile("/proc/uptime"), ' ').at(0);
    return utils::stringToInt(v);
}

double ExecutionUnitLinux::getCpuTime() const{
    std::vector<std::string> statValues = getStatFields();

    double uTime = (double) utils::stringToInt(statValues.at(PROC_STAT_UTIME));
    double sTime = (double) utils::stringToInt(statValues.at(PROC_STAT_STIME));
    double cpuTime = uTime + sTime;
    if(0){ //TODO:
        double cuTime = (double) utils::stringToInt(statValues.at(PROC_STAT_CUTIME));
        double csTime = (double) utils::stringToInt(statValues.at(PROC_STAT_CSTIME));
        cpuTime += (cuTime + csTime);
    }
    return cpuTime;
}

bool ExecutionUnitLinux::isActive() const{
    return utils::existsDirectory(_path);
}

std::vector<std::string> ExecutionUnitLinux::getStatFields() const{
    return utils::split(utils::readFirstLineFromFile(_path + "stat"), ' ');
}

bool ExecutionUnitLinux::getCoreUsage(double& coreUsage) const{
    try{
        double upTime = getUpTime();
        if(upTime > _lastUpTime){
            coreUsage = (((getCpuTime() - _lastCpuTime) / _hertz) / (upTime - _lastUpTime)) * 100.0;
        }else{
            coreUsage = 0;
        }
        return true;
    }catch(const std::runtime_error& exc){
        if(!isActive()){
            return false;
        }else{
            throw exc;
        }
    }
}

bool ExecutionUnitLinux::resetCoreUsage(){
    try{
        _lastUpTime = getUpTime();
        _lastCpuTime = getCpuTime();
        return true;
    }catch(const std::runtime_error& exc){
        if(!isActive()){
            return false;
        }else{
            throw exc;
        }
    }
}

bool ExecutionUnitLinux::getVirtualCoreId(topology::VirtualCoreId& virtualCoreId) const{
    try{
        virtualCoreId = utils::stringToInt(getStatFields().at(PROC_STAT_PROCESSOR));
        return true;
    }catch(const std::runtime_error& exc){
        if(!isActive()){
            return false;
        }else{
            throw exc;
        }
    }
}

bool ExecutionUnitLinux::moveToCpu(const topology::Cpu* cpu) const{
    std::vector<topology::VirtualCore*> v = cpu->getVirtualCores();
    return moveToVirtualCores(std::vector<const topology::VirtualCore*>(v.begin(), v.end()));
}

bool ExecutionUnitLinux::moveToPhysicalCore(const topology::PhysicalCore* physicalCore) const{
    std::vector<topology::VirtualCore*> v = physicalCore->getVirtualCores();
    return moveToVirtualCores(std::vector<const topology::VirtualCore*>(v.begin(), v.end()));
}

bool ExecutionUnitLinux::moveToVirtualCore(const topology::VirtualCore* virtualCore) const{
    std::vector<const topology::VirtualCore*> v;
    v.push_back(virtualCore);
    return moveToVirtualCores(v);
}

bool ExecutionUnitLinux::moveToVirtualCores(const std::vector<const topology::VirtualCore*> virtualCores) const{
    std::string virtualCoresList = "";
    std::vector<const topology::VirtualCore*>::const_iterator it = virtualCores.begin();

    while(it != virtualCores.end()){
        virtualCoresList.append(utils::intToString((*it)->getVirtualCoreId()));
        if(it + 1 != virtualCores.end()){
            virtualCoresList.append(",");
        }
        ++it;
    }

    try{
        utils::executeCommand("taskset -p " +
                               std::string(tasksetAll()?" -a ":"") +
                               "-c " + virtualCoresList + " " +
                               utils::intToString(_id) +
                               " > /dev/null");
        return true;
    }catch(const std::runtime_error& exc){
        if(!isActive()){
            return false;
        }else{
            throw exc;
        }
    }
}

static std::vector<Pid> getExecutionUnitsIdentifiers(std::string path){
    std::vector<std::string> procStr = utils::getFilesNamesInDir(path, false, true);
    std::vector<Pid> identifiers;
    for(size_t i = 0; i < procStr.size(); i++){
        std::string procId = procStr.at(i);
        if(utils::isNumber(procId)){
            identifiers.push_back(utils::stringToInt(procId));
        }
    }
    return identifiers;
}

ThreadHandlerLinux::ThreadHandlerLinux(Pid pid, Pid tid):
        ExecutionUnitLinux(tid, "/proc/" + utils::intToString(pid) + "/task/" + utils::intToString(tid) + "/"){
    ;
}

bool ThreadHandlerLinux::tasksetAll() const{
    return false;
}

ProcessHandlerLinux::ProcessHandlerLinux(Pid pid):
        ExecutionUnitLinux(pid, "/proc/" + utils::intToString(pid) + "/"), _pid(pid){
    ;
}

bool ProcessHandlerLinux::tasksetAll() const{
    return true;
}

std::vector<Pid> ProcessHandlerLinux::getActiveThreadsIdentifiers() const{
    return getExecutionUnitsIdentifiers(getPath() + "/task/");
}

ThreadHandler* ProcessHandlerLinux::getThreadHandler(Pid tid) const{
    return new ThreadHandlerLinux(_pid, tid);
}

void ProcessHandlerLinux::releaseThreadHandler(ThreadHandler* thread) const{
    if(thread){
        delete thread;
    }
}

ProcessesManagerLinux::ProcessesManagerLinux(){
    ;
}

std::vector<Pid> ProcessesManagerLinux::getActiveProcessesIdentifiers() const{
    return getExecutionUnitsIdentifiers("/proc");
}

ProcessHandler* ProcessesManagerLinux::getProcessHandler(Pid pid) const{
    return new ProcessHandlerLinux(pid);
}

void ProcessesManagerLinux::releaseProcessHandler(ProcessHandler* process) const{
    if(process){
        delete process;
    }
}

}
}
