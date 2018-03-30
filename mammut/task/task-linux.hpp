#ifndef MAMMUT_PROCESS_LINUX_HPP_
#define MAMMUT_PROCESS_LINUX_HPP_

#include <map>
#include <atomic>
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
    bool move(const std::vector<topology::VirtualCore*>& virtualCores) const;
    bool move(const std::vector<const topology::VirtualCore*>& virtualCores) const;
    virtual bool move(const std::vector<topology::VirtualCoreId>& virtualCoresIds) const = 0;
    bool isActive() const;
};

class ThreadHandlerLinux: public ThreadHandler, public ExecutionUnitLinux{
private:
    TaskId _tid;
    std::string getSetPriorityIdentifiers() const;
public:
    ThreadHandlerLinux(TaskId pid, TaskId tid);
    bool move(const std::vector<topology::VirtualCoreId>& virtualCoresIds) const;
};

class ProcessHandlerLinux;

class ThrottlerThread: public utils::Thread{
private:
    std::atomic_flag _run;
    utils::LockPthreadMutex _lock;
    std::vector<std::pair<const ProcessHandlerLinux*, double>> _processesToAdd;
    std::vector<const ProcessHandlerLinux*> _processesToRemove;
    double _sumPercentages;
    std::map<const ProcessHandlerLinux*, double> _throttlingValues;

    void addProcess(const std::pair<const ProcessHandlerLinux*, double>&);
    void removeProcess(const ProcessHandlerLinux*);
public:
    ThrottlerThread();
    void run();
    /**
     * Asks the throttler thread to throttle a specific process by a specific
     * percentage.
     * @param process The process handler.
     * @param percentage The percentage of time the process should execute on
     * the CPU. E.g. if percentage == 30, each second of the execution the
     * process will run for 0.3 seconds and sleep for 0.7 seconds.
     * This value must be included in the range ]0, 100].
     * @return True if the throttling is succesful, false if the sum of throttling
     * percentages of the different processes exceeds 100%.
     */
    bool throttle(const ProcessHandlerLinux* process, double percentage);

    /**
     * Removes throttling from a specified process.
     * @param process The process handler.
     */
    void removeThrottling(const ProcessHandlerLinux *process);
    void stop();
};

class ProcessHandlerLinux: public ProcessHandler, public ExecutionUnitLinux{
private:
    TaskId _pid;
    ThrottlerThread& _throttlerThread;
    std::string getSetPriorityIdentifiers() const;
#ifdef WITH_PAPI
    bool _countersAvailable;
    int _eventSet;
    long long * _values;
    long long * _oldValues;
#endif
public:
    explicit ProcessHandlerLinux(TaskId pid,
                                 ThrottlerThread& throttlerThread);
    ~ProcessHandlerLinux();
    std::vector<TaskId> getActiveThreadsIdentifiers() const;
    ThreadHandler* getThreadHandler(TaskId tid) const;
    void releaseThreadHandler(ThreadHandler* thread) const;
    bool move(const std::vector<topology::VirtualCoreId>& virtualCoresIds) const;
    bool getInstructions(double& instructions);
    bool resetInstructions();
    bool getAndResetInstructions(double& instructions);
    bool throttle(double percentage);
    bool removeThrottling();
    bool sendSignal(int signal) const;
};

class ProcessesManagerLinux: public TasksManager{
private:
    ThrottlerThread _throttler;
public:
    ProcessesManagerLinux();
    ~ProcessesManagerLinux();
    std::vector<TaskId> getActiveProcessesIdentifiers() const;
    ProcessHandler* getProcessHandler(TaskId pid);
    void releaseProcessHandler(ProcessHandler* process) const;
    ThreadHandler* getThreadHandler(TaskId pid, TaskId tid) const;
    ThreadHandler* getThreadHandler() const;
    void releaseThreadHandler(ThreadHandler* thread) const;
};


}
}


#endif /* MAMMUT_PROCESS_LINUX_HPP_ */
