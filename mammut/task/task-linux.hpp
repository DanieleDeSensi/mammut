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
    std::vector<std::string> getStatFields() const;
    virtual std::string getSetPriorityIdentifiers() const = 0;
protected:
    bool isActive() const;
public:
    ExecutionUnitLinux(TaskId id, std::string path);
    TaskId getId() const;
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
    bool move(const std::vector<topology::VirtualCore*> virtualCores) const;
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
#ifdef WITH_PAPI
    int _eventSet;
    long long * _values;
    long long * _oldValues;
#endif
public:
    ProcessHandlerLinux(TaskId pid);
    ~ProcessHandlerLinux();
    std::vector<TaskId> getActiveThreadsIdentifiers() const;
    ThreadHandler* getThreadHandler(TaskId tid) const;
    void releaseThreadHandler(ThreadHandler* thread) const;
    bool move(const std::vector<topology::VirtualCoreId> virtualCoresIds) const;
    bool getCycles(double& cycles);
    bool resetCycles();
    bool getAndResetCycles(double& cycles);
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
