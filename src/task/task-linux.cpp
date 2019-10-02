#include <mammut/task/task-linux.hpp>

#include "unistd.h"

#ifdef WITH_PAPI
#include <papi.h>
#define NUM_PAPI_EVENTS 2
#endif

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>

namespace mammut{
extern SimulationParameters simulationParameters;
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
                                               throw; \
                                           } \
                                       } \
                                   }while(0)\

ExecutionUnitLinux::ExecutionUnitLinux(TaskId id, std::string path):_id(id),
    _path(path), _hertz(utils::getClockTicksPerSecond()){
    resetCoreUsage();
}

TaskId ExecutionUnitLinux::getId() const{
    return _id;
}

std::string ExecutionUnitLinux::getPath() const{
    return _path;
}

double ExecutionUnitLinux::getUpTime() const{
    return utils::stringToInt(utils::split(utils::readFirstLineFromFile(std::string("/proc/uptime")), ' ').at(0));
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
    return utils::existsFile(_path);
    //return !kill(_id, 0);
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

bool ExecutionUnitLinux::move(const std::vector<const topology::VirtualCore*>& virtualCores) const{
    std::vector<topology::VirtualCore*> tmp;
    tmp.reserve(virtualCores.size());
    for(size_t i = 0; i < virtualCores.size(); i++){
        tmp.push_back((topology::VirtualCore*) virtualCores[i]);
    }
    return move(tmp);
}

bool ExecutionUnitLinux::move(const std::vector<topology::VirtualCore*>& virtualCores) const{
    std::vector<topology::VirtualCoreId> virtualCoresIds;
    for(size_t i = 0; i < virtualCores.size(); i++){
        virtualCoresIds.push_back(virtualCores.at(i)->getVirtualCoreId());
    }
    return move(virtualCoresIds);
}

bool ExecutionUnitLinux::getVirtualCoreIds(std::vector<topology::VirtualCoreId>& vcs) const{
    cpu_set_t set;
    CPU_ZERO(&set);
    if(sched_getaffinity(_id, sizeof(cpu_set_t), &set) == -1){
        return false;
    }
    vcs.clear();
    for(long i = 0; i < sysconf(_SC_NPROCESSORS_ONLN); i++){
        if(CPU_ISSET(i, &set)){
            vcs.push_back(i);
        }
    }
    return true;
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
        ExecutionUnitLinux(tid, "/proc/" + utils::intToString(pid) + "/task/" +
                           utils::intToString(tid) + "/"),
        _tid(tid){
    ;
}

std::string ThreadHandlerLinux::getSetPriorityIdentifiers() const{
    return utils::intToString(_tid);
}

bool ThreadHandlerLinux::move(const std::vector<topology::VirtualCoreId>& virtualCoresIds) const{
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

ProcessHandlerLinux::ProcessHandlerLinux(TaskId pid,
                                         ThrottlerThread& throttlerThread):
        ExecutionUnitLinux(pid, "/proc/" + utils::intToString(pid) + "/"),
        _pid(pid),
        _throttlerThread(throttlerThread){
#if defined(WITH_PAPI)
    if(!isActive()){return;}
    _countersAvailable = true;
    _eventSet = PAPI_NULL;
    _values = new long long[NUM_PAPI_EVENTS];
    _oldValues = new long long[NUM_PAPI_EVENTS];
    const PAPI_hw_info_t *hw_info;
    const PAPI_component_info_t *cmpinfo;
    int retval;
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    if(retval != PAPI_VER_CURRENT){
        _countersAvailable = false;
        return;
    }
    if((cmpinfo = PAPI_get_component_info(0)) == NULL){
        _countersAvailable = false;
        return;
    }
    if(cmpinfo->attach == 0){
        _countersAvailable = false;
        return;
    }
    hw_info = PAPI_get_hardware_info();
    if(hw_info == NULL){
        _countersAvailable = false;
        return;
    }

    retval = PAPI_create_eventset(&_eventSet);
    if(retval != PAPI_OK){
        _countersAvailable = false;
        return;
    }
    retval = PAPI_assign_eventset_component(_eventSet, 0);
    if(retval != PAPI_OK){
        _countersAvailable = false;
        return;
    }
    PAPI_option_t opt;
    memset(&opt, 0x0, sizeof(PAPI_option_t));
    opt.inherit.inherit = PAPI_INHERIT_ALL;
    opt.inherit.eventset = _eventSet;
    if((retval = PAPI_set_opt(PAPI_INHERIT, &opt)) != PAPI_OK){
        _countersAvailable = false;
        return;
    }

    retval = PAPI_attach(_eventSet, (unsigned long) _pid);
    if(retval != PAPI_OK){
        _countersAvailable = false;
        return;
    }
    retval = PAPI_add_event(_eventSet, PAPI_TOT_INS);
    if(retval != PAPI_OK){
        _countersAvailable = false;
        return;
    }
    retval = PAPI_add_event(_eventSet, PAPI_TOT_CYC);
    if(retval != PAPI_OK){
        _countersAvailable = false;
        return;
    }
    retval = PAPI_start(_eventSet);
    if(retval != PAPI_OK){
        _countersAvailable = false;
        return;
    }
    resetInstructions();
#endif
}

ProcessHandlerLinux::~ProcessHandlerLinux(){
#if defined(WITH_PAPI)
    PAPI_stop(_eventSet, _values);
    PAPI_cleanup_eventset(_eventSet);
    PAPI_destroy_eventset(&_eventSet);
    delete[] _values;
    delete[] _oldValues;
#endif
    _throttlerThread.removeThrottling(this);
}


bool ProcessHandlerLinux::move(const std::vector<topology::VirtualCoreId>& virtualCoresIds) const{
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
    std::vector<TaskId> tids;
    try{
        tids = getExecutionUnitsIdentifiers(getPath() + "/task/");
    }catch(const std::runtime_error& exc){
        if(isActive()){
            throw;
        }
    }
    return tids;
}

ThreadHandler* ProcessHandlerLinux::getThreadHandler(TaskId tid) const{
    return new ThreadHandlerLinux(_pid, tid);
}

void ProcessHandlerLinux::releaseThreadHandler(ThreadHandler* thread) const{
    if(thread){
        delete thread;
    }
}

bool ProcessHandlerLinux::getInstructions(double& instructions){
#if defined(WITH_PAPI)
    if(!_countersAvailable){return isActive();}
    int retval;
    retval = PAPI_read(_eventSet, _values);
    if(!isActive()){
        return false;
    }
    if(retval != PAPI_OK){
        throw std::runtime_error("PAPI_read " + retval);
    }
    instructions = _values[0] - _oldValues[0];
    return true;
#else
    throw std::runtime_error("Please define WITH_PAPI if you want to get "
                             "performance counters.");
#endif
}

bool ProcessHandlerLinux::resetInstructions(){
#if defined(WITH_PAPI)
    if(!_countersAvailable){return isActive();}
    int retval;
    retval = PAPI_read(_eventSet, _oldValues);
    if(!isActive()){
        return false;
    }
    if(retval != PAPI_OK){
        throw std::runtime_error("PAPI_read " + retval);
    }
    return true;
#else
    throw std::runtime_error("Please define WITH_PAPI if you want to get "
                             "performance counters.");
#endif
}

bool ProcessHandlerLinux::getAndResetInstructions(double& instructions){
#if defined(WITH_PAPI)
    if(!getInstructions(instructions)){return false;}
    for(size_t i = 0; i < NUM_PAPI_EVENTS; i++){
        _oldValues[i] = _values[i];
    }
    return true;
#else
    throw std::runtime_error("Please define WITH_PAPI if you want to get "
                             "instructions.");
#endif
}

bool ProcessHandlerLinux::throttle(double percentage){
    if(_pid == getpid()){
        throw std::runtime_error("Trottling cannot be applied on the calling process.");
    }
    if(percentage <= 0 || percentage > 100){
        throw std::runtime_error("Throttling percentage must be in range ]0, 100].");
    }
    if(!_throttlerThread.throttle(this, percentage)){
        std::ostringstream strs;
        strs << _pid;
        throw std::runtime_error("Throttling on pid " + utils::intToString(_pid) +
                                 " cannot "
                                 "be performed since the sum of throttling "
                                 "percentages for the different processes "
                                 "exceeds 100%.");
    }
    return isActive();
}

bool ProcessHandlerLinux::removeThrottling(){
    _throttlerThread.removeThrottling(this);
    return isActive();
}

bool ProcessHandlerLinux::sendSignal(int signal) const{
    if(kill(_pid, signal) == -1){
        if(!isActive()){
            return false;
        }else{
            throw std::runtime_error("Impossible to send signal " +
                                     utils::intToString(signal) + " to process " +
                                     utils::intToString(_pid));
        }
    }
    return true;
}

#define MAMMUT_THROTTLING_INTERVAL_DEFAULT_MICROSECS 100000

ThrottlerThread::ThrottlerThread():
    _run(ATOMIC_FLAG_INIT), 
    _throttlingInterval(MAMMUT_THROTTLING_INTERVAL_DEFAULT_MICROSECS), 
    _sumPercentages(0){
        _run.test_and_set();
}

void ThrottlerThread::addProcess(const std::pair<const ProcessHandlerLinux*, double>& process){
    auto it = _throttlingValues.find(process.first);
    double percentage = process.second;
    if(it != _throttlingValues.end()){
        // Already present, just update the percentage.
        it->second = percentage;
    }else if(_sumPercentages + percentage <= 100.0){
        _throttlingValues.insert(process);
        _sumPercentages += percentage;
    }
}

void ThrottlerThread::removeProcess(const ProcessHandlerLinux* process){
    auto it = _throttlingValues.find(process);
    if(it != _throttlingValues.end()){
        _sumPercentages -= it->second;
        _throttlingValues.erase(it);
    }
}

void ThrottlerThread::run(){
    while(_run.test_and_set()){
        _lock.lock();
        for(auto it : _processesToAdd){
            addProcess(it);
        }
        _processesToAdd.clear();

        for(auto it : _processesToRemove){
            removeProcess(it);
        }
        _processesToRemove.clear();
        _lock.unlock();

        double sleptMicroSecs = 0;
        std::vector<const ProcessHandlerLinux*> terminated;
        ulong throttlingIntervalLocal = _throttlingInterval;
        // Selectively start/stop processes
        for(auto it : _throttlingValues){
            if(it.first->sendSignal(SIGCONT)){
                double activeMicroSecs = (it.second / 100.0) * throttlingIntervalLocal;
                usleep(activeMicroSecs);
                sleptMicroSecs += activeMicroSecs;
                if(!it.first->sendSignal(SIGSTOP)){
                    terminated.push_back(it.first);
                }
            }else{
                terminated.push_back(it.first);
            }
        }
        // Remove processes that terminated in this iteration.
        _lock.lock();
        for(auto it : terminated){
            _throttlingValues.erase(_throttlingValues.find(it));
        }
        _lock.unlock();
        // Sleep until the end of this interval
        usleep(throttlingIntervalLocal - sleptMicroSecs);
    }

    for(auto it : _throttlingValues){
        it.first->sendSignal(SIGCONT);
    }
}

bool ThrottlerThread::throttle(const ProcessHandlerLinux *process, double percentage){
    utils::ScopedLock sLock(_lock);
    if(_throttlingValues.find(process) == _throttlingValues.end() &&
       _sumPercentages + percentage > 100.0){
        // Adding throttling for this process would lead to having
        // a sum of percentages higher than 100.0
        return false;
    }else{
        _processesToAdd.push_back(std::pair<const ProcessHandlerLinux*, double>(process, percentage));
        return true;
    }
}

void ThrottlerThread::removeThrottling(const ProcessHandlerLinux *process){
    utils::ScopedLock sLock(_lock);
    _processesToRemove.push_back(process);
}

void ThrottlerThread::setThrottlingInterval(ulong throttlingInterval){
    _throttlingInterval = throttlingInterval;
}

void ThrottlerThread::stop(){
    _run.clear();
}

ProcessesManagerLinux::ProcessesManagerLinux(){
    _throttler.start();
}

ProcessesManagerLinux::~ProcessesManagerLinux(){
    _throttler.stop();
    _throttler.join();
}

std::vector<TaskId> ProcessesManagerLinux::getActiveProcessesIdentifiers() const{
    return getExecutionUnitsIdentifiers("/proc");
}

ProcessHandler* ProcessesManagerLinux::getProcessHandler(TaskId pid){
    return new ProcessHandlerLinux(pid, _throttler);
}

void ProcessesManagerLinux::releaseProcessHandler(ProcessHandler* process) const{
    if(process){
        delete process;
    }
}

void ProcessesManagerLinux::setThrottlingInterval(ulong throttlingInterval){
    _throttler.setThrottlingInterval(throttlingInterval);
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
