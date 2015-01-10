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

ExecutionUnitLinux::ExecutionUnitLinux(std::string path):_path(path), _hertz(sysconf(_SC_CLK_TCK)){
    resetCoreUsage();
}

std::string ExecutionUnitLinux::getPath() const{
    return _path;
}

double ExecutionUnitLinux::getUpTime(){
    std::string v;
    v = utils::split(utils::readFirstLineFromFile("/proc/uptime"), ' ').at(0);
    return utils::stringToInt(v);
}

double ExecutionUnitLinux::getCpuTime(){
    std::vector<std::string> statValues = utils::split(utils::readFirstLineFromFile(_path + "stat"), ' ');
    std::string v;
    double cpuTime;

    v = statValues.at(PROC_STAT_UTIME);
    double uTime = (double) utils::stringToInt(v);

    v = statValues.at(PROC_STAT_STIME);
    double sTime = (double) utils::stringToInt(v);

    cpuTime = uTime + sTime;
    if(0){
        v = statValues.at(PROC_STAT_CUTIME);
        double cuTime = (double) utils::stringToInt(v);

        v = statValues.at(PROC_STAT_CSTIME);
        double csTime = (double) utils::stringToInt(v);
        cpuTime += (cuTime + csTime);
    }
    return cpuTime;
}

bool ExecutionUnitLinux::isActive(){
    return utils::existsDirectory(_path);
}

bool ExecutionUnitLinux::getCoreUsage(double& coreUsage){
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

bool ExecutionUnitLinux::mapToCore(){
    try{
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

ThreadHandlerLinux::ThreadHandlerLinux(Pid pid, Tid tid):
        ExecutionUnitLinux("/proc/" + utils::intToString(pid) + "/task/" + utils::intToString(tid) + "/"){
    ;
}

ProcessHandlerLinux::ProcessHandlerLinux(Pid pid):
        ExecutionUnitLinux("/proc/" + utils::intToString(pid) + "/"), _pid(pid){
    ;
}

std::vector<Tid> ProcessHandlerLinux::getActiveThreadsIdentifiers() const{
    return getExecutionUnitsIdentifiers(getPath());
}

ThreadHandler* ProcessHandlerLinux::getThreadHandler(Tid tid) const{
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
