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
    std::string _path;
    double _hertz;
    double _lastCpuTime;
    double _lastUpTime;
    double getUpTime();
    double getCpuTime();
    bool isActive();
public:
    ExecutionUnitLinux(std::string path);
    std::string getPath() const;
    bool getCoreUsage(double& coreUsage);
    bool resetCoreUsage();
    bool mapToCore();
};

class ThreadHandlerLinux: public ThreadHandler, public ExecutionUnitLinux{
public:
    ThreadHandlerLinux(Pid pid, Tid tid);
};

class ProcessHandlerLinux: public ProcessHandler, public ExecutionUnitLinux{
private:
    Pid _pid;
public:
    ProcessHandlerLinux(Pid pid);
    std::vector<Tid> getActiveThreadsIdentifiers() const;
    ThreadHandler* getThreadHandler(Tid tid) const;
    void releaseThreadHandler(ThreadHandler* thread) const;
};

class ProcessesManagerLinux: public ProcessesManager{
public:
    ProcessesManagerLinux();
    std::vector<Pid> getActiveProcessesIdentifiers() const;
    ProcessHandler* getProcessHandler(Pid pid) const;
    void releaseProcessHandler(ProcessHandler* process) const;
};


}
}


#endif /* MAMMUT_PROCESS_LINUX_HPP_ */
