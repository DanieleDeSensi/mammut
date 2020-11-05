#include <mammut/energy/energy-linux.hpp>

#include "../external/odroid-smartpower-linux/smartgauge.hpp"

#include "cmath"
#include "errno.h"
#include "fcntl.h"
#include "fstream"
#include "sstream"
#include "stdexcept"
#include "string"
#include "cstring"
#include "unistd.h"
#include "iostream"
/* RAPL UNIT BITMASK */
#define POWER_UNIT_OFFSET 0
#define POWER_UNIT_MASK 0x0F

#define ENERGY_UNIT_OFFSET 0x08
#define ENERGY_UNIT_MASK 0x1F00

#define TIME_UNIT_OFFSET 0x10
#define TIME_UNIT_MASK 0xF000

#define JOULES_IN_WATTHOUR 3600.0

using namespace mammut::utils;

namespace mammut{
namespace energy{

CounterAmesterLinux::CounterAmesterLinux(string jlsSensor, string wtsSensor):
        _sensorJoules(jlsSensor), _sensorWatts(wtsSensor),
        _lastValue(0), _lastTimestamp(0), _timeOffset(0){
    ;
}

bool CounterAmesterLinux::init(){
    bool r = _sensorJoules.exists();
    if(r){
        AmesterResult ar = _sensorJoules.readSum();
        _lastValue = ar.value;
        _lastTimestamp = ar.timestamp;
        _timeOffset = getMillisecondsTime() - ar.timestamp;
    }
    return r;
}

Joules CounterAmesterLinux::getAdjustedValue(){
    AmesterResult ar = _sensorJoules.readSum();
    ulong localTimestamp = ar.timestamp + _timeOffset;
    AmesterResult arw = _sensorWatts.readSum();
    ar.value += (arw.value * ((getMillisecondsTime() - localTimestamp) / 1000.0));
    return ar.value;
}

Joules CounterAmesterLinux::getJoules(){
    return getAdjustedValue() - _lastValue;
}

void CounterAmesterLinux::reset(){
    _lastValue = getAdjustedValue();
}

CounterPlugSmartPower2Linux::CounterPlugSmartPower2Linux():_usbFile(NULL), _cumulativeJoules(0){
    ;
}

CounterPlugSmartPower2Linux::~CounterPlugSmartPower2Linux(){
    if(_usbFile){
        fclose(_usbFile);
        _usbFile = NULL;
    }
}

bool CounterPlugSmartPower2Linux::CounterPlugSmartPower2Linux::init(){
    char* usbName = getenv("MAMMUT_SMARTPOWER2_PATH");
    if(!usbName){
        return false;
    }
    _usbFile = fopen(usbName, "r");    
    if (_usbFile == NULL) {
        return false;
    }
    _lastTimestamp = getMillisecondsTime();    
    return true;
}

double CounterPlugSmartPower2Linux::getWatts(){
  if(!_usbFile){
    return 0;
  }

  int count=0,len=0;
  char * data = NULL, *line = NULL, buff[50];
  
  buff[0]='\n';
  while( buff[0] == '\n' ) fgets(buff, 50, (FILE*)_usbFile);//there is an empty line between power data
  
  line = buff;
  len = strlen(buff);
  
if (len > 28){ //So the first two values are voltage.
    line = strchr(buff, ','); //removing the first value
    line+=1;//remove the  comma
  }

  if (len >= 23){ //accepts power above 9  

    data = strtok (line,",");
    while (data != NULL && count < 2)
    {
      data  = strtok (NULL, ","); // getting the power value
      count++;
    }
  }else
    return 0;

  double p = atof(data);

  return p;
}

Joules CounterPlugSmartPower2Linux::getJoules(){
    double watts = getWatts();
    ulong now = getMillisecondsTime();
    _cumulativeJoules += watts*((now - _lastTimestamp)/1000.0);
    _lastTimestamp = now;
    return _cumulativeJoules;
}

void CounterPlugSmartPower2Linux::reset(){
    _cumulativeJoules = 0;
    _lastTimestamp = getMillisecondsTime();
}

CounterPlugSmartGaugeLinux::CounterPlugSmartGaugeLinux():_lastValue(0){
    _sg = new SmartGauge();
}

CounterPlugSmartGaugeLinux::~CounterPlugSmartGaugeLinux(){
    delete _sg;
}

bool CounterPlugSmartGaugeLinux::init(){
    bool available = _sg->initDevice();
    if(available){
        _lastValue = getJoulesAbs();
    }
    return available;
}

Joules CounterPlugSmartGaugeLinux::getJoulesAbs(){
    return (_sg->getWattHour() * JOULES_IN_WATTHOUR);
}

Joules CounterPlugSmartGaugeLinux::getJoules(){
    return getJoulesAbs() - _lastValue;
}

void CounterPlugSmartGaugeLinux::reset(){
    _lastValue = getJoulesAbs();
}

#define COUNTER_FILE_LINUX_NAME "/tmp/counter_power.csv"

CounterPlugFileLinux::CounterPlugFileLinux():
  _lastTimestamp(getMillisecondsTime()), _cumulativeJoules(0){
	;
}

bool CounterPlugFileLinux::init(){
	bool available = existsFile(COUNTER_FILE_LINUX_NAME);
	if(available){
		_lastTimestamp = getMillisecondsTime();
	}
	return available;
}

double CounterPlugFileLinux::getWatts(){
	std::string s = readFirstLineFromFile(COUNTER_FILE_LINUX_NAME);
	std::vector<std::string> values = split(s, ',');
	return atof(values[0].c_str());
}

Joules CounterPlugFileLinux::getJoules(){
	double watts = getWatts();
	ulong now = getMillisecondsTime();
	_cumulativeJoules += watts*((now - _lastTimestamp)/1000.0);
	_lastTimestamp = now;
	return _cumulativeJoules;
}

void CounterPlugFileLinux::reset(){
	_cumulativeJoules = 0;
	_lastTimestamp = getMillisecondsTime();
}

#include <sys/ioctl.h>
#define INA231_IOCGREG      _IOR('i', 1, SensorInaData *)
#define INA231_IOCSSTATUS   _IOW('i', 2, SensorInaData *)
#define INA231_IOCGSTATUS   _IOR('i', 3, SensorInaData *)

SensorIna::SensorIna(const char *name):_available(false), _name(name){;}

bool SensorIna::init(){
  _fd = open(_name, O_RDWR);
  if(_fd < 0)
    return false;

  if(ioctl(_fd, INA231_IOCGSTATUS, &_data) < 0)
    throw std::runtime_error("CounterPlugINA: ioctl error (0)");

  if (!_data.enable){
      _data.enable = 1;
  }
  if(ioctl(_fd, INA231_IOCSSTATUS, &_data) < 0){
     throw std::runtime_error("CounterPlugINA: ioctl error (1)");
  }
  _available = true;
  return true;
}

SensorIna::~SensorIna(){
  if(_available){
    if (_data.enable){
        _data.enable = 0;
    }
    ioctl(_fd, INA231_IOCSSTATUS, &_data);
    close(_fd);
  }
}

double SensorIna::getWatts(){
  if(ioctl(_fd, INA231_IOCGREG, &_data) < 0)
    throw std::runtime_error("CounterPlugINA: ioctl error (2)");
  return (_data.cur_uW / 1000.0) / 1000.0;
}

bool CounterPlugINALinux::init(){
  _lastRead = getMillisecondsTime();
  return _sensorA7.init() && _sensorA15.init();
}

CounterPlugINALinux::CounterPlugINALinux():
  _sensorA7("/dev/sensor_kfc"), _sensorA15("/dev/sensor_arm"), _cumulativeJoules(0){
  ;
}

CounterPlugINALinux::~CounterPlugINALinux(){

}

Joules CounterPlugINALinux::getJoules(){
  double now = getMillisecondsTime();
  Joules j = (_sensorA7.getWatts() + _sensorA15.getWatts()) * ((now - _lastRead)/1000.0);
  _cumulativeJoules += j;
  _lastRead = now;
  return j;
}

void CounterPlugINALinux::reset(){
  _cumulativeJoules = 0;
  _lastRead = getMillisecondsTime();
}

bool CounterMemoryRaplLinux::init(){
    if(_ccl){
        return true;
    }else{
        return false;
    }
}

CounterMemoryRaplLinux::CounterMemoryRaplLinux():_ccl(new CounterCpusLinuxMsr()){
    if(!_ccl->init()){
        delete _ccl;
        _ccl = NULL;
    }
}

Joules CounterMemoryRaplLinux::getJoules(){
    if(_ccl){
        return _ccl->getJoulesDramAll();
    }else{
        return 0;
    }
}

void CounterMemoryRaplLinux::reset(){
    if(_ccl){
        _ccl->reset();
    }
}

CounterCpusLinuxRefresher::CounterCpusLinuxRefresher(CounterCpusLinux *counter):_counter(counter){
    ;
}

void CounterCpusLinuxRefresher::run(){
    double sleepingIntervalMs = (_counter->getWrappingInterval() / 2) * 1000;
    while(!_counter->_stopRefresher.timedWait(sleepingIntervalMs)){
        _counter->getJoulesComponentsAll();
    }
}

CounterCpusLinux::CounterCpusLinux():
  CounterCpus(topology::Topology::getInstance()),
  _stopRefresher(){
  ;
}

JoulesCpu CounterCpusLinux::getJoulesComponents(topology::CpuId cpuId){
  return JoulesCpu(getJoulesCpu(cpuId), getJoulesCores(cpuId), getJoulesGraphic(cpuId), getJoulesDram(cpuId));
}

bool CounterCpusLinuxMsr::hasCoresCounter(topology::Cpu* cpu){
  if(_family == CPU_FAMILY_INTEL){
    uint64_t dummy;
    return Msr(cpu->getVirtualCore()->getVirtualCoreId()).read(MSR_PP0_ENERGY_STATUS_INTEL, dummy) && dummy > 0;
  }else if(_family == CPU_FAMILY_AMD){
    return false; // TODO Return true and implement per-core readings
  }
  return false;
}

bool CounterCpusLinuxMsr::hasGraphicCounter(topology::Cpu* cpu){
  if(_family == CPU_FAMILY_INTEL){
    uint64_t dummy;
    return Msr(cpu->getVirtualCore()->getVirtualCoreId()).read(MSR_PP1_ENERGY_STATUS_INTEL, dummy) && dummy > 0;
  }else{
    return false;
  }
}

bool CounterCpusLinuxMsr::hasDramCounter(topology::Cpu* cpu){
  if(_family == CPU_FAMILY_INTEL){
    uint64_t dummy;
    return Msr(cpu->getVirtualCore()->getVirtualCoreId()).read(MSR_DRAM_ENERGY_STATUS_INTEL, dummy) && dummy > 0;
  }else{
    return false;
  }
}

bool CounterCpusLinuxMsr::isCpuIntelSupported(topology::Cpu* cpu){
    Msr msr(cpu->getVirtualCore()->getVirtualCoreId());
    uint64_t dummy, dummy2, dummy3; // 3 different variables to avoid cppcheck warnings.
    return !cpu->getFamily().compare("6") &&
           !cpu->getVendorId().compare(0, 12, "GenuineIntel") &&
           msr.available() &&
           msr.read(MSR_RAPL_POWER_UNIT_INTEL, dummy) && dummy &&
           msr.read(MSR_PKG_POWER_INFO_INTEL, dummy2) && dummy2 &&
           msr.read(MSR_PKG_ENERGY_STATUS_INTEL, dummy3);
}

bool CounterCpusLinuxMsr::isCpuAMDSupported(topology::Cpu* cpu){
    Msr msr(cpu->getVirtualCore()->getVirtualCoreId());
    uint64_t dummy, dummy3; // 3 different variables to avoid cppcheck warnings.
    return !cpu->getFamily().compare("23") &&
           !cpu->getVendorId().compare(0, 12, "AuthenticAMD") &&
           msr.available() &&
           msr.read(MSR_RAPL_POWER_UNIT_AMD, dummy) && dummy &&
           msr.read(MSR_PKG_ENERGY_STATUS_AMD, dummy3);
}

bool CounterCpusLinuxMsr::isCpuSupported(topology::Cpu* cpu){
    return isCpuIntelSupported(cpu) || isCpuAMDSupported(cpu);
}

CounterCpusLinuxMsr::CounterCpusLinuxMsr():
        _initialized(false),
        _lock(),
        _refresher(NULL),
        _msrs(NULL),
        _maxId(0),
        _powerPerUnit(0),
        _energyPerUnit(0),
        _timePerUnit(0),
        _thermalSpecPower(0),
        _joulesCpus(NULL),
        _lastReadCountersCpu(NULL),
        _lastReadCountersCores(NULL),
        _lastReadCountersGraphic(NULL),
        _lastReadCountersDram(NULL),
        _hasJoulesCores(false),
        _hasJoulesGraphic(false),
        _hasJoulesDram(false){
    ;
}

bool CounterCpusLinuxMsr::init(){
    if(!_cpus[0]->getVendorId().compare(0, 12, "GenuineIntel")){
      _family = CPU_FAMILY_INTEL;
    }else if(!_cpus[0]->getVendorId().compare(0, 12, "AuthenticAMD")){
      _family = CPU_FAMILY_AMD;
    }else{
      return false;
    }

    for(size_t i = 0; i < _cpus.size(); i++){
        if(!isCpuSupported(_cpus.at(i))){
            return false;
        }	
    }

    _initialized = true;
    _refresher = new CounterCpusLinuxRefresher(this);

    /**
     * I have one msr for each CPU. Since I have no guarantee that the CPU
     * identifiers are contiguous, I find their maximum id to decide
     * how long the array of msrs should be.
     * For example, if a have a CPU with id 1 and a CPU with id 3, then
     * I will allocate an array of 4 positions.
     * In msrs[1] I will have the msr associated to CPU 1 and in msrs[3]
     * I will have the msr associated to CPU 3.
     */
    _maxId = 0;
    topology::Cpu* cpu;
    for(size_t i = 0; i < _cpus.size(); i++){
        cpu = _cpus.at(i);
        if(cpu->getCpuId() > _maxId){
            _maxId = cpu->getCpuId();
        }
    }

    _msrs = new Msr*[_maxId + 1];
    for(size_t i = 0; i < _cpus.size(); i++){
        cpu = _cpus.at(i);
        _msrs[cpu->getCpuId()] = new Msr(cpu->getVirtualCore()->getVirtualCoreId());
        if(!_msrs[cpu->getCpuId()]->available()){
            throw std::runtime_error("Impossible to open msr for CPU " +
                                     intToString(cpu->getCpuId()));
        }
    }

    _joulesCpus = new JoulesCpu[_maxId + 1];

    _lastReadCountersCpu = new uint32_t[_maxId + 1];
    _lastReadCountersCores = new uint32_t[_maxId + 1];
    _lastReadCountersGraphic = new uint32_t[_maxId + 1];
    _lastReadCountersDram = new uint32_t[_maxId + 1];

    /*
     * Calculate the units used. We suppose to have the same units
     * for all the CPUs. So we can read the units of any of the CPUs.
     */
    uint64_t result;
    if(_family == CPU_FAMILY_INTEL){
      _msrs[_maxId]->read(MSR_RAPL_POWER_UNIT_INTEL, result);
    }else{
      _msrs[_maxId]->read(MSR_RAPL_POWER_UNIT_AMD, result);
    }
    _powerPerUnit = pow(0.5,(double)(result&0xF));
    _energyPerUnit = pow(0.5,(double)((result>>8)&0x1F));
    _timePerUnit = pow(0.5,(double)((result>>16)&0xF));
    if(_family == CPU_FAMILY_INTEL){
      _msrs[_maxId]->read(MSR_PKG_POWER_INFO_INTEL, result);
      _thermalSpecPower = _powerPerUnit*(double)(result&0x7FFF);
    }else if(_family == CPU_FAMILY_AMD){
      _thermalSpecPower = 180.0; // See https://lkml.org/lkml/2018/7/17/1275
    }

    _hasJoulesCores = true;
    _hasJoulesDram = true;
    _hasJoulesGraphic = true;
    for(size_t i = 0; i < _cpus.size(); i++){
    	if(!hasCoresCounter(_cpus.at(i))){
            _hasJoulesCores = false;
    	}
        if(!hasDramCounter(_cpus.at(i))){
            _hasJoulesDram = false;
        }
        if(!hasGraphicCounter(_cpus.at(i))){
            _hasJoulesGraphic = false;
        }
    }

    reset();
    _refresher->start();
    return true;
}

CounterCpusLinuxMsr::~CounterCpusLinuxMsr(){
    if(_initialized){
        _stopRefresher.notifyAll();
        _refresher->join();
        delete _refresher;

        for(size_t i = 0; i < _maxId + 1; i++){
            if(_msrs[i]){
                delete _msrs[i];
            }
        }
        delete[] _msrs;

        delete[] _joulesCpus;
        delete[] _lastReadCountersCpu;
        delete[] _lastReadCountersCores;
        delete[] _lastReadCountersGraphic;
        delete[] _lastReadCountersDram;
    }
}

uint32_t CounterCpusLinuxMsr::readEnergyCounter(topology::CpuId cpuId, int which){
    switch(which){
        case MSR_PKG_ENERGY_STATUS_INTEL:
        case MSR_PP0_ENERGY_STATUS_INTEL:
        case MSR_PP1_ENERGY_STATUS_INTEL:
        case MSR_DRAM_ENERGY_STATUS_INTEL:
        case MSR_PKG_ENERGY_STATUS_AMD:
        case MSR_PP0_ENERGY_STATUS_AMD:{
            uint64_t result;
            if(!_msrs[cpuId]->read(which, result) || !result){
                throw std::runtime_error("Fatal error. Counter has been created but registers are not present.");
            }
            return result & 0xFFFFFFFF;
        }break;
        default:{
            throw std::runtime_error("Invalid energy counter specification.");
        }
    }
}

double CounterCpusLinuxMsr::getWrappingInterval(){
    return ((double) 0xFFFFFFFF) * _energyPerUnit / _thermalSpecPower;
}

static uint32_t deltaDiff(uint32_t c1, uint32_t c2){
    if(c2 > c1){
        return c2 - c1;
    }else{
        return (((uint32_t)0xFFFFFFFF) - c1) + (uint32_t)1 + c2;
    }
}

void CounterCpusLinuxMsr::updateCounter(topology::CpuId cpuId, double& joules, uint32_t& lastReadCounter, uint32_t counterType){
    uint32_t currentCounter = readEnergyCounter(cpuId, counterType);
    joules += ((double) deltaDiff(lastReadCounter, currentCounter)) * _energyPerUnit;
    lastReadCounter = currentCounter;
}

Joules CounterCpusLinuxMsr::getJoulesCpu(topology::CpuId cpuId){
    ScopedLock sLock(_lock);
    if(_family == CPU_FAMILY_INTEL){
      updateCounter(cpuId, _joulesCpus[cpuId].cpu, _lastReadCountersCpu[cpuId], MSR_PKG_ENERGY_STATUS_INTEL);
    }else if(_family == CPU_FAMILY_AMD){
      updateCounter(cpuId, _joulesCpus[cpuId].cpu, _lastReadCountersCpu[cpuId], MSR_PKG_ENERGY_STATUS_AMD);
    }
    return _joulesCpus[cpuId].cpu;
}

Joules CounterCpusLinuxMsr::getJoulesCores(topology::CpuId cpuId){
    if(hasJoulesCores()){
        ScopedLock sLock(_lock);
        if(_family == CPU_FAMILY_INTEL){
          updateCounter(cpuId, _joulesCpus[cpuId].cores, _lastReadCountersCores[cpuId], MSR_PP0_ENERGY_STATUS_INTEL);
        }else{
          // TODO Read joules from each individual core and sum them.
        }
        return _joulesCpus[cpuId].cores;
    }else{
        return 0;
    }
}

Joules CounterCpusLinuxMsr::getJoulesGraphic(topology::CpuId cpuId){
    if(hasJoulesGraphic()){
        ScopedLock sLock(_lock);
        if(_family == CPU_FAMILY_INTEL){
          updateCounter(cpuId, _joulesCpus[cpuId].graphic, _lastReadCountersGraphic[cpuId], MSR_PP1_ENERGY_STATUS_INTEL);
          return _joulesCpus[cpuId].graphic;
        }
    }
    return 0;
}

Joules CounterCpusLinuxMsr::getJoulesDram(topology::CpuId cpuId){
    if(hasJoulesDram()){
        ScopedLock sLock(_lock);
        if(_family == CPU_FAMILY_INTEL){
          updateCounter(cpuId, _joulesCpus[cpuId].dram, _lastReadCountersDram[cpuId], MSR_DRAM_ENERGY_STATUS_INTEL);
          return _joulesCpus[cpuId].dram;
        }
    }
    return 0;
}

void CounterCpusLinuxMsr::reset(){
    ScopedLock sLock(_lock);
    for(size_t i = 0; i < _cpus.size(); i++){
        if(_family == CPU_FAMILY_INTEL){
          _lastReadCountersCpu[i] = readEnergyCounter(i, MSR_PKG_ENERGY_STATUS_INTEL);
        }else if(_family == CPU_FAMILY_AMD){
          _lastReadCountersCpu[i] = readEnergyCounter(i, MSR_PKG_ENERGY_STATUS_AMD);
        }
        if(hasJoulesCores()){
          _lastReadCountersCores[i] = readEnergyCounter(i, MSR_PP0_ENERGY_STATUS_INTEL);
        }
        if(hasJoulesGraphic()){
            _lastReadCountersGraphic[i] = readEnergyCounter(i, MSR_PP1_ENERGY_STATUS_INTEL);
        }
        if(hasJoulesDram()){
            _lastReadCountersDram[i] = readEnergyCounter(i, MSR_DRAM_ENERGY_STATUS_INTEL);
        }
        _joulesCpus[i].cpu = 0;
        _joulesCpus[i].cores = 0;
        _joulesCpus[i].graphic = 0;
        _joulesCpus[i].dram = 0;
    }
}

bool CounterCpusLinuxMsr::hasJoulesCores(){
    return _hasJoulesCores;
}

bool CounterCpusLinuxMsr::hasJoulesDram(){
    return _hasJoulesDram;
}

bool CounterCpusLinuxMsr::hasJoulesGraphic(){
    return _hasJoulesGraphic;
}

CounterCpusLinuxSysFs::CounterCpusLinuxSysFs():
  _initialized(false),
  _lock(),
  _refresher(NULL),
  _idCores(-1),
  _idGraphic(-1),
  _idDram(-1){
  ;
}

#define RAPL_SYSFS_PREFIX std::string("/sys/class/powercap/intel-rapl/intel-rapl:")

bool CounterCpusLinuxSysFs::init(){
  for(size_t i = 0; i < _cpus.size(); i++){
      if(!utils::existsFile(RAPL_SYSFS_PREFIX + utils::intToString(i) + "/energy_uj")){
          return false;
      }
  }
  _initialized = true;
  _refresher = new CounterCpusLinuxRefresher(this);
  int sub = 0;
  while(utils::existsFile(RAPL_SYSFS_PREFIX + "0/intel-rapl:0:" + utils::intToString(sub) + "/name")){
    std::string name = utils::readFirstLineFromFile(RAPL_SYSFS_PREFIX + "0/intel-rapl:0:" + utils::intToString(sub) + "/name");
    if(name == "core"){
      _idCores = sub;
    }else if(name == "dram"){
      _idDram = sub;
    }else if(name == "uncore"){ // TODO Not sure this is the correct name.
      _idGraphic = sub;
    }
    sub += 1;
  }
  _lastCpu.resize(_cpus.size());
  _lastCores.resize(_cpus.size());
  _lastDram.resize(_cpus.size());
  _lastGraphic.resize(_cpus.size());
  _joulesCpus.resize(_cpus.size());
  _maxValue = atof(utils::readFirstLineFromFile(RAPL_SYSFS_PREFIX + "0/max_energy_range_uj").c_str()) / 1000000.0;
  reset();
  _refresher->start();
  return true;
}

Joules CounterCpusLinuxSysFs::read(topology::CpuId cpuId, Joules& cumulative, int sub, std::vector<Joules> &last){
  ScopedLock sLock(_lock);
  Joules now;
  if(sub != -1){
    now = atof(utils::readFirstLineFromFile(RAPL_SYSFS_PREFIX + utils::intToString(cpuId) + "/intel-rapl:" + utils::intToString(cpuId) + ":" + utils::intToString(sub) + "/energy_uj").c_str()) / 1000000.0;
  }else{
    now = atof(utils::readFirstLineFromFile(RAPL_SYSFS_PREFIX + utils::intToString(cpuId) + "/energy_uj").c_str()) / 1000000.0;
  }
  Joules r;
  if(last[cpuId] > now){
    r = _maxValue - last[cpuId] + now;
  }else{
    r = now - last[cpuId];
  }
  cumulative += r;
  last[cpuId] = now;
  return cumulative;
}

Joules CounterCpusLinuxSysFs::getJoulesCpu(topology::CpuId cpuId){
  return read(cpuId, _joulesCpus[cpuId].cpu, -1, _lastCpu);
}

Joules CounterCpusLinuxSysFs::getJoulesCores(topology::CpuId cpuId){
  if(_idCores != -1){
    return read(cpuId, _joulesCpus[cpuId].cores, _idCores, _lastCores);
  }else{
    return 0;
  }
}

Joules CounterCpusLinuxSysFs::getJoulesGraphic(topology::CpuId cpuId){
  if(_idGraphic != -1){
    return read(cpuId, _joulesCpus[cpuId].graphic, _idGraphic, _lastGraphic);
  }else{
    return 0;
  }
}

Joules CounterCpusLinuxSysFs::getJoulesDram(topology::CpuId cpuId){
  if(_idDram != -1){
    return read(cpuId, _joulesCpus[cpuId].dram, _idDram, _lastDram);
  }else{
    return 0;
  }
}

bool CounterCpusLinuxSysFs::hasJoulesCores(){
  return _idCores != -1;
}

bool CounterCpusLinuxSysFs::hasJoulesDram(){
  return _idDram != -1;
}

bool CounterCpusLinuxSysFs::hasJoulesGraphic(){
  return _idGraphic != -1;
}

void CounterCpusLinuxSysFs::reset(){
  for(size_t i = 0; i < _cpus.size(); i++){
    getJoulesCpu(i);
    getJoulesCores(i);
    getJoulesGraphic(i);
    getJoulesDram(i);
    _joulesCpus[i].cpu = 0;
    _joulesCpus[i].cores = 0;
    _joulesCpus[i].graphic = 0;
    _joulesCpus[i].dram = 0;
  }
}

CounterCpusLinuxSysFs::~CounterCpusLinuxSysFs(){
  if(_initialized){
      _stopRefresher.notifyAll();
      _refresher->join();
      delete _refresher;
  }
}

PowerCapperLinux::PowerCapperLinux(CounterType type):PowerCapper(type), _good(false){
#ifdef HAVE_RAPLCAP
  switch(_type){
  case COUNTER_CPUS:{
    _zone = RAPLCAP_ZONE_PACKAGE;
  }break;
  case COUNTER_MEMORY:{
    _zone = RAPLCAP_ZONE_DRAM;
  }break;
  case COUNTER_PLUG:{
    _zone = RAPLCAP_ZONE_PSYS;
  }break;
  default:{
      throw std::runtime_error("PowerCapperLinux: Unknown type.");
  }
  }
#else
  throw std::runtime_error("If you want to use the power capper, please build Mammut with -DENABLE_RAPLCAP=ON.");
#endif
}

PowerCapperLinux::~PowerCapperLinux(){
#ifdef HAVE_RAPLCAP
  if(_good){
     raplcap_destroy(&_rc);
  }
#endif
}

bool PowerCapperLinux::init(){
#ifdef HAVE_RAPLCAP
  _sockets = raplcap_get_num_sockets(NULL);
  if(_sockets == 0){
    return false;
  }

  if(raplcap_init(&_rc)) {
    return false;
  }

  for(size_t i = 0; i < _sockets; i++){
    if(!raplcap_is_zone_supported(&_rc, i, _zone)){
      return false;
    }
  }
  _good = true;
  return true;
#else
  throw std::runtime_error("If you want to use the power capper, please build Mammut with -DENABLE_RAPLCAP=ON.");
#endif
}

std::vector<std::pair<PowerCap, PowerCap>> PowerCapperLinux::get() const{
  std::vector<std::pair<PowerCap, PowerCap>> r;
  for(size_t i = 0; i < _sockets; i++){
    r.push_back(get(i));
  }
  return r;
}

std::pair<PowerCap, PowerCap> PowerCapperLinux::get(uint socketId) const{
  std::pair<PowerCap, PowerCap> r;
  r.first = get(socketId, 0);
  r.second = get(socketId, 1);
  return r;
}

PowerCap PowerCapperLinux::get(uint socketId, uint windowId) const{
#ifdef HAVE_RAPLCAP
  raplcap_limit zero, one;
  if(raplcap_get_limits(&_rc, socketId, _zone, &zero, &one)){
    throw std::runtime_error("raplcap_get_limits");
  }
  PowerCap r;
  if(windowId == 0){
    r.value = zero.watts;
    r.window = zero.seconds;
  }else{
    r.value = one.watts;
    r.window = one.seconds;
  }
  r.preparedOnly = !raplcap_is_zone_enabled(&_rc, socketId, _zone);
  return r;
#else
  throw std::runtime_error("If you want to use the power capper, please build Mammut with -DENABLE_RAPLCAP=ON.");
#endif
}

void PowerCapperLinux::set(PowerCap cap){
  double wattsPerSocket = cap.value / _sockets;
  cap.value = wattsPerSocket;
  for(size_t i = 0; i < _sockets; i++){
    set(i, cap);
  }
}

void PowerCapperLinux::set(uint socketId, PowerCap cap){
  for(size_t i = 0; i < 2; i++){
      set(i, socketId, cap);
  }
}

void PowerCapperLinux::set(uint windowId, uint socketId, PowerCap cap){
#ifdef HAVE_RAPLCAP
  raplcap_limit lim;
  lim.watts = cap.value;
  lim.seconds = cap.window;
  int r = 0;
  if(windowId == 0){
    r = raplcap_set_limits(&_rc, socketId, _zone, &lim, NULL);
  }else{
    r = raplcap_set_limits(&_rc, socketId, _zone, NULL, &lim);
  }
  if(r){
    throw std::runtime_error("raplcap_set_limits");
  }
  if(raplcap_set_zone_enabled(&_rc, socketId, _zone, !cap.preparedOnly)){
    throw std::runtime_error("raplcap_set_zone_enabled");
  }
#else
  throw std::runtime_error("If you want to use the power capper, please build Mammut with -DENABLE_RAPLCAP=ON.");
#endif
}

}
}
