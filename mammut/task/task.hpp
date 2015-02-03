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

#ifndef MAMMUT_PROCESS_HPP_
#define MAMMUT_PROCESS_HPP_

#include <mammut/module.hpp>
#include <mammut/topology/topology.hpp>

#include <sys/resource.h>
#define MAMMUT_PROCESS_PRIORITY_MIN 0
#define MAMMUT_PROCESS_PRIORITY_MAX (PRIO_MAX - PRIO_MIN)

namespace mammut{
namespace task{

typedef pid_t TaskId;

class Task{
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
    virtual bool getCoreUsage(double& coreUsage) const = 0; //TODO: Time AND percentage

    /**
     * Resets the counter for core usage percentage computation.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool resetCoreUsage() = 0;

    /**
     * Gets the current priority of this execution unit.
     * @param priority The current priority of this execution unit. The higher
     *        is the value, the higher is the priority. It will be in
     *        the range [MAMMUT_PROCESS_PRIORITY_MIN, MAMMUT_PROCESS_PRIORITY_MAX]
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool getPriority(uint& priority) const = 0;

    /**
     * Sets the priority of this execution unit.
     * NOTE: If executed on a process, the priority of all its thread will be changed too.
     * @param priority The priority of this execution unit. The higher
     *        is the value, the higher is the priority. It must be in
     *        the range [MAMMUT_PROCESS_PRIORITY_MIN, MAMMUT_PROCESS_PRIORITY_MAX]
     * @return If false is returned, the priority value is outside the allowed range or
     *         this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool setPriority(uint priority) const = 0;

    /**
     * Gets the identifier of the virtual core on which this unit is currently running.
     * @param virtualCoreId The identifier of the virtual core on which this unit is currently running.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool getVirtualCoreId(topology::VirtualCoreId& virtualCoreId) const = 0;

    /**
     * Move this execution unit on a specified CPU.
     * NOTE: If executed on a process, all its threads will be moved too.
     * @param cpu The CPU on which this execution unit must be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(const topology::Cpu* cpu) const = 0;

    /**
     * Move this execution unit on a specified physical core.
     * NOTE: If executed on a process, all its threads will be moved too.
     * @param physicalCore The physical core on which this execution unit must be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(const topology::PhysicalCore* physicalCore) const = 0;

    /**
     * Move this execution unit on a specified virtual core.
     * NOTE: If executed on a process, all its threads will be moved too.
     * @param virtualCore The virtual core on which this execution unit must be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(const topology::VirtualCore* virtualCore) const = 0;

    /**
     * Move this execution unit on a specified virtual core.
     * NOTE: If executed on a process, all its threads will be moved too.
     * @param virtualCoreId The identifier of the virtual core on which this execution unit must
     *                      be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(topology::VirtualCoreId virtualCoreId) const = 0;

    /**
     * Move this execution unit on a set of specified virtual cores.
     * @param virtualCores The virtual cores on which this execution unit must be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(const std::vector<const topology::VirtualCore*> virtualCores) const = 0;

    /**
     * Move this execution unit on a set of specified virtual cores.
     * @param virtualCoresIds The identifiers of the virtual cores on which this execution unit
     *                        must be moved.
     * @return If false is returned, this execution unit is no more active and the call failed.
     *         Otherwise, true is returned.
     */
    virtual bool move(const std::vector<topology::VirtualCoreId> virtualCoresIds) const = 0;

    virtual ~Task(){;}
};

class ThreadHandler: public virtual Task{
public:
    virtual ~ThreadHandler(){;}
};

class ProcessHandler: public virtual Task{
public:
    /**
     * Returns a list of active thread identifiers on this process.
     * @return A vector of active thread identifiers on this process.
     */
    virtual std::vector<TaskId> getActiveThreadsIdentifiers() const = 0;

    /**
     * Returns the handler associated to a specific thread.
     * @param tid The thread identifier.
     * @return The handler associated to a specific thread or
     *         NULL if the thread doesn't exists. The obtained
     *         handler must be released with releaseThreadHandler call.
     */
    virtual ThreadHandler* getThreadHandler(TaskId tid) const = 0;

    /**
     * Releases the handler obtained through getThreadHandler call.
     * @param thread The thread handler.
     */
    virtual void releaseThreadHandler(ThreadHandler* thread) const = 0;

    virtual ~ProcessHandler(){;}
};

class TasksManager: public Module{
    MAMMUT_MODULE_DECL(TasksManager)
public:
    /**
     * Returns a list of active processes identifiers.
     * @return A vector of active processes identifiers.
     */
    virtual std::vector<TaskId> getActiveProcessesIdentifiers() const = 0;

    /**
     * Returns the handler associated to a specific process.
     * @param pid The process identifier.
     * @return The handler associated to a specific process or
     *         NULL if the process doesn't exists. The obtained
     *         handler must be released with releaseProcessHandler call.
     */
    virtual ProcessHandler* getProcessHandler(TaskId pid) const = 0;

    /**
     * Releases the handler obtained through getProcessHandler call.
     * @param process The process handler.
     */
    virtual void releaseProcessHandler(ProcessHandler* process) const = 0;

    /**
     * Returns the handler associated to a specific thread.
     * @param pid The process identifier.
     * @param tid The thread identifier.
     * @return The handler associated to a specific thread or
     *         NULL if the thread doesn't exists. The obtained
     *         handler must be released with releaseThreadHandler call.
     */
    virtual ThreadHandler* getThreadHandler(TaskId pid, TaskId tid) const = 0;

    /**
     * Returns the handler associated to the calling thread.
     * @return The handler associated to the calling thread.
     *         The obtained handler must be released with
     *         releaseThreadHandler call.
     */
    virtual ThreadHandler* getThreadHandler() const = 0;

    /**
     * Releases the handler obtained through getThreadHandler call.
     * @param thread The thread handler.
     */
    virtual void releaseThreadHandler(ThreadHandler* thread) const = 0;
};

}
}

#endif /* MAMMUT_PROCESS_HPP_ */
