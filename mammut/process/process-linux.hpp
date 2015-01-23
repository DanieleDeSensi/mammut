/*
 * process-linux.hpp
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

#ifndef MAMMUT_PROCESS_LINUX_HPP_
#define MAMMUT_PROCESS_LINUX_HPP_

#include <mammut/process/process.hpp>

namespace mammut{
namespace process{

class ExecutionUnitLinux: public virtual ExecutionUnit{
private:
    Pid _id;
    std::string _path;
    double _hertz;
    double _lastCpuTime;
    double _lastUpTime;
    double getUpTime() const;
    double getCpuTime() const;
    bool isActive() const;
    std::vector<std::string> getStatFields() const;
    virtual bool allThreadsMove() const = 0;
    virtual std::string getSetPriorityIdentifiers() const = 0;
protected:
public:
    ExecutionUnitLinux(Pid id, std::string path);
    std::string getPath() const;
    bool getCoreUsage(double& coreUsage) const;
    bool resetCoreUsage();
    bool getPriority(uint& priority) const;
    bool setPriority(uint priority) const;
    bool getVirtualCoreId(topology::VirtualCoreId& virtualCoreId) const;
    bool moveToCpu(const topology::Cpu* cpu) const;
    bool moveToPhysicalCore(const topology::PhysicalCore* physicalCore) const;
    bool moveToVirtualCore(const topology::VirtualCore* virtualCore) const;
    bool moveToVirtualCores(const std::vector<const topology::VirtualCore*> virtualCores) const;
};

class ThreadHandlerLinux: public ThreadHandler, public ExecutionUnitLinux{
private:
    Pid _tid;
    bool allThreadsMove() const;
    std::string getSetPriorityIdentifiers() const;
public:
    ThreadHandlerLinux(Pid pid, Pid tid);
};

class ProcessHandlerLinux: public ProcessHandler, public ExecutionUnitLinux{
private:
    Pid _pid;
    bool allThreadsMove() const;
    std::string getSetPriorityIdentifiers() const;
public:
    ProcessHandlerLinux(Pid pid);
    std::vector<Pid> getActiveThreadsIdentifiers() const;
    ThreadHandler* getThreadHandler(Pid tid) const;
    void releaseThreadHandler(ThreadHandler* thread) const;
};

class ProcessesManagerLinux: public ProcessesManager{
public:
    ProcessesManagerLinux();
    std::vector<Pid> getActiveProcessesIdentifiers() const;
    ProcessHandler* getProcessHandler(Pid pid) const;
    void releaseProcessHandler(ProcessHandler* process) const;
    ThreadHandler* getThreadHandler(Pid pid, Pid tid) const;
    void releaseThreadHandler(ThreadHandler* thread) const;
};


}
}


#endif /* MAMMUT_PROCESS_LINUX_HPP_ */
