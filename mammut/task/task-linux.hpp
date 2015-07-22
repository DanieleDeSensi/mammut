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

#include "task.hpp"

namespace mammut{
namespace task{

class ExecutionUnitLinux: public virtual Task{
private:
    TaskId _id;
    std::string _path;
    double _hertz;
    double _lastCpuTime;
    double _lastUpTime;
    double getUpTime() const;
    double getCpuTime() const;
    bool isActive() const;
    std::vector<std::string> getStatFields() const;
    virtual std::string getSetPriorityIdentifiers() const = 0;
protected:
public:
    ExecutionUnitLinux(TaskId id, std::string path);
    std::string getPath() const;
    bool getCoreUsage(double& coreUsage) const;
    bool resetCoreUsage();
    bool getPriority(uint& priority) const;
    bool setPriority(uint priority) const;
    bool getVirtualCoreId(topology::VirtualCoreId& virtualCoreId) const;
    bool move(const topology::Cpu* cpu) const;
    bool move(const topology::PhysicalCore* physicalCore) const;
    bool move(const topology::VirtualCore* virtualCore) const;
    bool move(topology::VirtualCoreId virtualCoreId) const;
    bool move(const std::vector<const topology::VirtualCore*> virtualCores) const;
    virtual bool move(const std::vector<topology::VirtualCoreId> virtualCoresIds) const = 0;
};

class ThreadHandlerLinux: public ThreadHandler, public ExecutionUnitLinux{
private:
    TaskId _tid;
    std::string getSetPriorityIdentifiers() const;
public:
    ThreadHandlerLinux(TaskId pid, TaskId tid);
    bool move(const std::vector<topology::VirtualCoreId> virtualCoresIds) const;
};

class ProcessHandlerLinux: public ProcessHandler, public ExecutionUnitLinux{
private:
    TaskId _pid;
    std::string getSetPriorityIdentifiers() const;
public:
    ProcessHandlerLinux(TaskId pid);
    std::vector<TaskId> getActiveThreadsIdentifiers() const;
    ThreadHandler* getThreadHandler(TaskId tid) const;
    void releaseThreadHandler(ThreadHandler* thread) const;
    bool move(const std::vector<topology::VirtualCoreId> virtualCoresIds) const;
};

class ProcessesManagerLinux: public TasksManager{
public:
    ProcessesManagerLinux();
    std::vector<TaskId> getActiveProcessesIdentifiers() const;
    ProcessHandler* getProcessHandler(TaskId pid) const;
    void releaseProcessHandler(ProcessHandler* process) const;
    ThreadHandler* getThreadHandler(TaskId pid, TaskId tid) const;
    ThreadHandler* getThreadHandler() const;
    void releaseThreadHandler(ThreadHandler* thread) const;
};


}
}


#endif /* MAMMUT_PROCESS_LINUX_HPP_ */
