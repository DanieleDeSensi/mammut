/*
 * process.hpp
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

#ifndef PROCESS_HPP_ //TODO: MAMMUT_PROCESS_HPP (Anche gli altri).
#define PROCESS_HPP_

#include <mammut/module.hpp>

namespace mammut{
namespace process{

typedef uint64_t Pid;
typedef uint64_t Tid;

class ExecutionUnit{
public:
    /**
     * Returns the percentage of time spent by this execution unit on a processing
     * core. The percentage is computed over a period of time spanning from the
     * last call of resetCoreUsage (or from the creation of this object) to the
     * moment of this call.
     * @param coreUsage The percentage of time spent by this execution unit on a processing
     * core.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool getCoreUsage(double& coreUsage) = 0;

    /**
     * Resets the counter for core usage percentage computation.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool resetCoreUsage() = 0;

    /**
     * Maps this execution unit to a specific virtual core.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool mapToCore() = 0;

    virtual ~ExecutionUnit(){;}
};

class ThreadHandler: public virtual ExecutionUnit{
public:
    virtual ~ThreadHandler(){;}
};

class ProcessHandler: public virtual ExecutionUnit{
public:
    /**
     * Returns a list of active thread identifiers on this process.
     * @return A vector of active thread identifiers on this process.
     */
    virtual std::vector<Tid> getActiveThreadsIdentifiers() const = 0;

    /**
     * Returns the handler associated to a specific thread.
     * @param tid The thread identifier.
     * @return The handler associated to a specific thread or
     *         NULL if the thread doesn't exists. The obtained
     *         handler must be released with releaseThreadHandler call.
     */
    virtual ThreadHandler* getThreadHandler(Tid tid) const = 0;

    /**
     * Releases the handler obtained through getThreadHandler call.
     * @param thread The thread handler.
     */
    virtual void releaseThreadHandler(ThreadHandler* thread) const = 0;

    virtual ~ProcessHandler(){;}
};

class ProcessesManager: public Module{
    MAMMUT_MODULE_DECL(ProcessesManager)
public:
    /**
     * Returns a list of active processes identifiers.
     * @return A vector of active processes identifiers.
     */
    virtual std::vector<Pid> getActiveProcessesIdentifiers() const = 0;

    /**
     * Returns the handler associated to a specific process.
     * @param pid The process identifier.
     * @return The handler associated to a specific process or
     *         NULL if the process doesn't exists. The obtained
     *         handler must be released with releaseProcessHandler call.
     */
    virtual ProcessHandler* getProcessHandler(Pid pid) const = 0;

    /**
     * Releases the handler obtained through getProcessHandler call.
     * @param process The process handler.
     */
    virtual void releaseProcessHandler(ProcessHandler* process) const = 0;
};

}
}

#endif /* PROCESS_HPP_ */
