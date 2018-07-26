#include "../task/task.hpp"
#include "../topology/topology-linux.hpp"
#include "../utils.hpp"

#include <cmath>
#include <fstream>
#include <stdexcept>
//#include <arch/x86/include/asm/processor.h>

using namespace mammut::utils;

namespace mammut{
extern SimulationParameters simulationParameters;
namespace topology{

TopologyLinux::TopologyLinux():Topology(){
    ;
}

void TopologyLinux::maximizeUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->maximizeUtilization();
    }
}

void TopologyLinux::resetUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->resetUtilization();
    }
}

std::string getTopologyPathFromVirtualCoreId(VirtualCoreId id){
    return simulationParameters.sysfsRootPrefix +
           "/sys/devices/system/cpu/cpu" + intToString(id) + "/topology/";
}

CpuLinux::CpuLinux(CpuId cpuId, std::vector<PhysicalCore*> physicalCores):
    Cpu(cpuId, physicalCores){
    ;
}

std::string CpuLinux::getCpuInfo(const std::string& infoName) const{
    std::ifstream infile(simulationParameters.sysfsRootPrefix +
                         "/proc/cpuinfo");
    if(!infile){
        throw std::runtime_error("Impossible to open /proc/cpuinfo.");
    }

    //TODO: Non una linea qualsiasi ma quella di questa cpu
    std::string line;
    while(std::getline(infile, line)){
        if(!line.compare(0, infoName.length(), infoName)){
            return ltrim(split(line, ':').at(1));
        }
    }
    return "";
}

std::string CpuLinux::getVendorId() const{
    return getCpuInfo("vendor_id");
}

std::string CpuLinux::getFamily() const{
    return getCpuInfo("cpu family");
}

std::string CpuLinux::getModel() const{
    return getCpuInfo("model");
}

void CpuLinux::maximizeUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->maximizeUtilization();
    }
}

void CpuLinux::resetUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->resetUtilization();
    }
}

PhysicalCoreLinux::PhysicalCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId,
                                     std::vector<VirtualCore*> virtualCores):
    PhysicalCore(cpuId, physicalCoreId, virtualCores){
    ;
}

void PhysicalCoreLinux::maximizeUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->maximizeUtilization();
    }
}

void PhysicalCoreLinux::resetUtilization() const{
    for(size_t i = 0; i < _virtualCores.size(); i++){
        _virtualCores.at(i)->resetUtilization();
    }
}

VirtualCoreIdleLevelLinux::VirtualCoreIdleLevelLinux(const VirtualCoreLinux& virtualCore, uint levelId):
    VirtualCoreIdleLevel(virtualCore.getVirtualCoreId(), levelId),
    _virtualCore(virtualCore),
    _path(simulationParameters.sysfsRootPrefix +
          "/sys/devices/system/cpu/cpu" + intToString(virtualCore.getVirtualCoreId()) +
          "/cpuidle/state" + intToString(levelId) + "/"){
    resetTime();
    resetCount();
}

std::string VirtualCoreIdleLevelLinux::getName() const{
    return readFirstLineFromFile(_path + "name");
}

std::string VirtualCoreIdleLevelLinux::getDesc() const{
    return readFirstLineFromFile(_path + "desc");
}

bool VirtualCoreIdleLevelLinux::isEnableable() const{
    return existsFile(_path + "disable");
}

bool VirtualCoreIdleLevelLinux::isEnabled() const{
    if(isEnableable()){
        return (readFirstLineFromFile(_path + "disable").compare("0") == 0);
    }else{
        return true;
    }
}

void VirtualCoreIdleLevelLinux::enable() const{
    writeFile(_path + "disable", "0");
}

void VirtualCoreIdleLevelLinux::disable() const{
    writeFile(_path + "disable", "1");
}

uint VirtualCoreIdleLevelLinux::getExitLatency() const{
    return stringToInt(readFirstLineFromFile(_path + "latency"));
}

uint VirtualCoreIdleLevelLinux::getConsumedPower() const{
    return stringToInt(readFirstLineFromFile(_path + "power"));
}

uint VirtualCoreIdleLevelLinux::getAbsoluteTime() const{
    return stringToInt(readFirstLineFromFile(_path + "time"));
}

uint VirtualCoreIdleLevelLinux::getTime() const{
    return getAbsoluteTime() - _lastAbsTime;
}

void VirtualCoreIdleLevelLinux::resetTime(){
    _lastAbsTime = getAbsoluteTime();
}

uint VirtualCoreIdleLevelLinux::getAbsoluteCount() const{
    return stringToInt(readFirstLineFromFile(_path + "usage"));
}

uint VirtualCoreIdleLevelLinux::getCount() const{
    return getAbsoluteCount() - _lastAbsCount;
}

void VirtualCoreIdleLevelLinux::resetCount(){
    _lastAbsCount = getAbsoluteCount();
}

SpinnerThread::SpinnerThread():_stop(false){
    srand(time(NULL));
}

void SpinnerThread::setStop(bool s){
    _lock.lock();
    _stop = s;
    _lock.unlock();
}

bool SpinnerThread::isStopped(){
    bool r;
    _lock.lock();
    r = _stop;
    _lock.unlock();
    return r;
}

void SpinnerThread::run(){
    double r = rand();
    while(!isStopped()){
        r = sin(r);
    };
    writeFile("/dev/null", intToString(r));
}

VirtualCoreLinux::VirtualCoreLinux(CpuId cpuId, PhysicalCoreId physicalCoreId, VirtualCoreId virtualCoreId):
            VirtualCore(cpuId, physicalCoreId, virtualCoreId),
            _hotplugFile(simulationParameters.sysfsRootPrefix +
                         "/sys/devices/system/cpu/cpu" + intToString(virtualCoreId) +
                         "/online"),
            _utilizationThread(new SpinnerThread()),
            _clkModMsr(virtualCoreId, O_RDWR){
    std::vector<std::string> levelsNames;
    if(existsDirectory(simulationParameters.sysfsRootPrefix +
                       "/sys/devices/system/cpu/cpu0/cpuidle")){
        levelsNames = getFilesNamesInDir(simulationParameters.sysfsRootPrefix +
                                         "/sys/devices/system/cpu/cpu" +
                                         intToString(getVirtualCoreId()) +
                                         "/cpuidle", false, true);
    }
    for(size_t i = 0; i < levelsNames.size(); i++){
        std::string levelName = levelsNames.at(i);
        if(levelName.compare(0, 5, "state") == 0){
            uint levelId = stringToInt(levelName.substr(5));
            _idleLevels.push_back(new VirtualCoreIdleLevelLinux(*this, levelId));
        }
    }
    resetIdleTime();

    uint possibleValues;
    CpuIdAsm cia(6);
    if(cia.EAX() && (1 << 5)){
        // Extended clock modulation available.
        _clkModLowBit = 0;
        _clkModStep = 6.25;
        possibleValues = 15;
    }else{
        _clkModLowBit = 1;
        _clkModStep = 12.5;
        possibleValues = 7;
    }

    for(size_t i = 0; i < possibleValues; i++){
        _clkModValues.push_back(_clkModStep*(i+1));
    }
    // Manually push 100, since it is not possible to
    // set it through the modulation bits (it corresponds
    // in disabling modulation).
    _clkModValues.push_back(100.0); 

    _clkModMsr.readBits(MSR_CLOCK_MODULATION, 4, _clkModLowBit, _clkModMsrOrig);
}

VirtualCoreLinux::~VirtualCoreLinux(){
    deleteVectorElements<VirtualCoreIdleLevel*>(_idleLevels);
    resetUtilization();
    delete _utilizationThread;
    _clkModMsr.writeBits(MSR_CLOCK_MODULATION, 4, _clkModLowBit, _clkModMsrOrig); // Rollback 
}

bool VirtualCoreLinux::hasFlag(const std::string& flagName) const{
    if(!existsFile(simulationParameters.sysfsRootPrefix + "/proc/cpuinfo")){
        return false;
    }
    std::vector<std::string> file = readFile(simulationParameters.sysfsRootPrefix +
                                             "/proc/cpuinfo");
    bool procFound = false;
    for(size_t i = 0; i < file.size(); i++){
        /** Find the section of this virtual core. **/
        if(!procFound && file.at(i).find("processor") == 0 &&
           file.at(i).find(" " + intToString(_virtualCoreId))){
            procFound = true;
            continue;
        }

        if(procFound){
            if(file.at(i).find("flags") == 0 &&
               (file.at(i).find(" " + flagName + " ") != std::string::npos ||
                file.at(i).find(" " + flagName + "\n") != std::string::npos )){
                return true;
            }
            /** We finished the section of this virtual core. **/
            if(file.at(i).find("processor") == 0){
                return false;
            }
        }
    }
    return false;
}

uint64_t VirtualCoreLinux::getAbsoluteTicks() const{
    uint64_t ticks = 0;
    utils::Msr msr(_virtualCoreId);
    if(msr.available() &&
       msr.read(MSR_TSC, ticks)){
        return ticks;
    }
    return 0;
}

void VirtualCoreLinux::maximizeUtilization() const{
    if(_utilizationThread->running()){
        /** Is useless for the moment. Is just a placeholder in case
         *  different utilization levels will be added in the future.
         **/
        resetUtilization();
    }

    _utilizationThread->setStop(false);
    _utilizationThread->start();
    std::pair<task::TaskId, task::TaskId> pidTid = _utilizationThread->getPidAndTid();
    auto tHandler = task::TasksManager::local();
    mammut::task::ThreadHandler* h = tHandler->getThreadHandler(pidTid.first, pidTid.second);

    h->move(this);
    h->setPriority(MAMMUT_PROCESS_PRIORITY_MAX);
    tHandler->releaseThreadHandler(h);
    task::TasksManager::release(tHandler);
}

void VirtualCoreLinux::resetUtilization() const{
    if(_utilizationThread->running()){
        _utilizationThread->setStop(true);
        _utilizationThread->join();
    }
}

double VirtualCoreLinux::getProcStatTime(ProcStatTimeType type) const{
    std::vector<std::string> lines = readFile(simulationParameters.sysfsRootPrefix +
                                              "/proc/stat");
    std::string line;
    double field = 0;
    bool found = false;
    for(size_t i = 0; i < lines.size(); i++){
        line = lines.at(i);
        if(line.find("cpu" + intToString(getVirtualCoreId())) != std::string::npos){
            found = true;
            field = stringToInt(split(line, ' ')[type]);
        }
    }

    if(!found){
        return -1;
    }else{
        return (field / getClockTicksPerSecond()) * MAMMUT_MICROSECS_IN_SEC;
    }
}

double VirtualCoreLinux::getAbsoluteIdleTime() const{
    return getProcStatTime(PROC_STAT_IDLE);
#if 0
    //TODO: Provide also this one
    /** Alternative way of computing the idle time. **/
    double r = 0;
    for(size_t i = 0; i < _idleLevels.size(); i++){
        r += (_idleLevels.at(i)->getAbsoluteTime());
    }
    return r;
#endif

}

double VirtualCoreLinux::getIdleTime() const{
    return getAbsoluteIdleTime() - _lastProcIdleTime;
}

void VirtualCoreLinux::resetIdleTime(){
    _lastProcIdleTime = getAbsoluteIdleTime();
}

bool VirtualCoreLinux::isHotPluggable() const{
    return existsFile(_hotplugFile);
}

bool VirtualCoreLinux::isHotPlugged() const{
    if(isHotPluggable()){
        std::string online = readFirstLineFromFile(_hotplugFile);
        return (stringToInt(online) > 0);
    }else{
        return true;
    }
}

void VirtualCoreLinux::hotPlug() const{
    if(isHotPluggable()){
        writeFile(_hotplugFile, "1");
    }
}

void VirtualCoreLinux::hotUnplug() const{
    if(isHotPluggable()){
        writeFile(_hotplugFile, "0");
    }
}

bool VirtualCoreLinux::hasClockModulation() const{
    return _clkModMsr.available();
}

std::vector<double> VirtualCoreLinux::getClockModulationValues() const{
    return _clkModValues;
}

void VirtualCoreLinux::setClockModulation(double value){
    if(!contains(_clkModValues, value)){
        throw std::runtime_error("Wrong modulation value. Please use only value returned by "
                                 "getClockModulationValues() call");
    }else{
        if(value == 100){
            _clkModMsr.writeBits(MSR_CLOCK_MODULATION, 4, _clkModLowBit, 0);
        }else{
            _clkModMsr.writeBits(MSR_CLOCK_MODULATION, 4, 4, 1);
            _clkModMsr.writeBits(MSR_CLOCK_MODULATION, 3, _clkModLowBit, floor(value/_clkModStep));
        }
    }
}

double VirtualCoreLinux::getClockModulation() const{
    uint64_t v = 0;
    _clkModMsr.readBits(MSR_CLOCK_MODULATION, 4, 4, v);
    if(!v){
        return 100;
    }
    _clkModMsr.readBits(MSR_CLOCK_MODULATION, 3, _clkModLowBit, v);
    return v*_clkModStep;
}

std::vector<VirtualCoreIdleLevel*> VirtualCoreLinux::getIdleLevels() const{
    return _idleLevels;
}

}
}
