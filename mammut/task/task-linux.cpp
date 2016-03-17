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

#include "task-linux.hpp"

#include "unistd.h"

namespace mammut{
namespace task{

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

#define EXECUTE_AND_CHECK_ACTIVE(COMMAND) do{ \
                                       try{ \
                                           COMMAND \
                                       }catch(const std::runtime_error& exc){ \
                                           if(!isActive()){ \
                                               return false; \
                                           }else{ \
                                               throw exc; \
                                           } \
                                       } \
                                   }while(0)\

ExecutionUnitLinux::ExecutionUnitLinux(TaskId id, std::string path):_id(id), _path(path), _hertz(utils::getClockTicksPerSecond()){
    resetCoreUsage();
}

std::string ExecutionUnitLinux::getPath() const{
    return _path;
}

double ExecutionUnitLinux::getUpTime() const{
    return utils::stringToInt(utils::split(utils::readFirstLineFromFile("/proc/uptime"), ' ').at(0));
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
    double upTime, cpuTime;
    EXECUTE_AND_CHECK_ACTIVE(upTime = getUpTime(); cpuTime = getCpuTime(););
    if(upTime > _lastUpTime){
        coreUsage = (((cpuTime - _lastCpuTime) / _hertz) / (upTime - _lastUpTime)) * 100.0;
    }else{
        coreUsage = 0;
    }
    return true;
}

bool ExecutionUnitLinux::resetCoreUsage(){
    EXECUTE_AND_CHECK_ACTIVE(_lastUpTime = getUpTime(); _lastCpuTime = getCpuTime(););
    return true;
}

bool ExecutionUnitLinux::getPriority(uint& priority) const{
    EXECUTE_AND_CHECK_ACTIVE(priority = -(PRIO_MIN + utils::stringToInt(getStatFields().at(PROC_STAT_NICE))););
    return true;
}

bool ExecutionUnitLinux::setPriority(uint priority) const{
    if(priority < MAMMUT_PROCESS_PRIORITY_MIN || priority > MAMMUT_PROCESS_PRIORITY_MAX){
        return false;
    }

    if(utils::executeCommand("renice -n " + utils::intToString(-(priority + PRIO_MIN)) +
                                   " -p " + getSetPriorityIdentifiers(), true)){
        return false;
    }
    return true;
}

bool ExecutionUnitLinux::getVirtualCoreId(topology::VirtualCoreId& virtualCoreId) const{
    EXECUTE_AND_CHECK_ACTIVE(virtualCoreId = utils::stringToInt(getStatFields().at(PROC_STAT_PROCESSOR)););
    return true;
}

bool ExecutionUnitLinux::move(const topology::Cpu* cpu) const{
    std::vector<topology::VirtualCore*> v = cpu->getVirtualCores();
    return move(std::vector<const topology::VirtualCore*>(v.begin(), v.end()));
}

bool ExecutionUnitLinux::move(const topology::PhysicalCore* physicalCore) const{
    std::vector<topology::VirtualCore*> v = physicalCore->getVirtualCores();
    return move(std::vector<const topology::VirtualCore*>(v.begin(), v.end()));
}

bool ExecutionUnitLinux::move(const topology::VirtualCore* virtualCore) const{
    std::vector<const topology::VirtualCore*> v;
    v.push_back(virtualCore);
    return move(v);
}

bool ExecutionUnitLinux::move(topology::VirtualCoreId virtualCoreId) const{
    std::vector<topology::VirtualCoreId> v;
    v.push_back(virtualCoreId);
    return move(v);
}

bool ExecutionUnitLinux::move(const std::vector<const topology::VirtualCore*> virtualCores) const{
    std::vector<topology::VirtualCoreId> virtualCoresIds;
    for(size_t i = 0; i < virtualCores.size(); i++){
        virtualCoresIds.push_back(virtualCores.at(i)->getVirtualCoreId());
    }
    return move(virtualCoresIds);
}

static std::vector<TaskId> getExecutionUnitsIdentifiers(std::string path){
    std::vector<std::string> procStr = utils::getFilesNamesInDir(path, false, true);
    std::vector<TaskId> identifiers;
    for(size_t i = 0; i < procStr.size(); i++){
        std::string procId = procStr.at(i);
        if(utils::isNumber(procId)){
            identifiers.push_back(utils::stringToInt(procId));
        }
    }
    return identifiers;
}

ThreadHandlerLinux::ThreadHandlerLinux(TaskId pid, TaskId tid):
        ExecutionUnitLinux(tid, "/proc/" + utils::intToString(pid) + "/task/" + utils::intToString(tid) + "/"),
        _tid(tid){
    ;
}

std::string ThreadHandlerLinux::getSetPriorityIdentifiers() const{
    return utils::intToString(_tid);
}

bool ThreadHandlerLinux::move(const std::vector<topology::VirtualCoreId> virtualCoresIds) const{
    cpu_set_t set;
    CPU_ZERO(&set);
    for(size_t i = 0; i < virtualCoresIds.size(); i++){
        CPU_SET(virtualCoresIds.at(i), &set);
    }

    if(sched_setaffinity(_tid, sizeof(cpu_set_t), &set) == -1){
        return false;
    }
    return true;
}

ProcessHandlerLinux::ProcessHandlerLinux(TaskId pid):
        ExecutionUnitLinux(pid, "/proc/" + utils::intToString(pid) + "/"), _pid(pid){
    ;
}

bool ProcessHandlerLinux::move(const std::vector<topology::VirtualCoreId> virtualCoresIds) const{
    cpu_set_t set;
    CPU_ZERO(&set);
    for(size_t i = 0; i < virtualCoresIds.size(); i++){
        CPU_SET(virtualCoresIds.at(i), &set);
    }

    if(sched_setaffinity(_pid, sizeof(cpu_set_t), &set) == -1){
        return false;
    }

    std::vector<TaskId> threads = getActiveThreadsIdentifiers();
    for(size_t i = 0; i < threads.size(); i++){
        sched_setaffinity(threads.at(i), sizeof(cpu_set_t), &set);
    }
    return true;
}

std::string ProcessHandlerLinux::getSetPriorityIdentifiers() const{
    std::vector<TaskId> ids = getActiveThreadsIdentifiers();
    std::string r;
    std::vector<TaskId>::const_iterator it = ids.begin();

    while(it != ids.end()){
        r.append(utils::intToString(*it) + " ");
        ++it;
    }
    return r;
}

std::vector<TaskId> ProcessHandlerLinux::getActiveThreadsIdentifiers() const{
    return getExecutionUnitsIdentifiers(getPath() + "/task/");
}

ThreadHandler* ProcessHandlerLinux::getThreadHandler(TaskId tid) const{
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

std::vector<TaskId> ProcessesManagerLinux::getActiveProcessesIdentifiers() const{
    return getExecutionUnitsIdentifiers("/proc");
}

ProcessHandler* ProcessesManagerLinux::getProcessHandler(TaskId pid) const{
    return new ProcessHandlerLinux(pid);
}

void ProcessesManagerLinux::releaseProcessHandler(ProcessHandler* process) const{
    if(process){
        delete process;
    }
}

ThreadHandler* ProcessesManagerLinux::getThreadHandler(TaskId pid, TaskId tid) const{
    return new ThreadHandlerLinux(pid, tid);
}

ThreadHandler* ProcessesManagerLinux::getThreadHandler() const{
    return new ThreadHandlerLinux(getpid(), utils::gettid());
}

void ProcessesManagerLinux::releaseThreadHandler(ThreadHandler* thread) const{
    if(thread){
        delete thread;
    }
}

}
}
