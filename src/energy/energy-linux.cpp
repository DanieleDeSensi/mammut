#include <mammut/energy/energy-linux.hpp>

#include "../external/odroid-smartpower-linux/smartgauge.hpp"

#include "cmath"
#include "errno.h"
#include "fcntl.h"
#include "fstream"
#include "sstream"
#include "stdexcept"
#include "string"
#include "unistd.h"

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

bool CounterMemoryRaplLinux::init(){
    if(_ccl){
        return true;
    }else{
        return false;
    }
}

CounterMemoryRaplLinux::CounterMemoryRaplLinux():_ccl(new CounterCpusLinux()){
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

CounterCpusLinuxRefresher::CounterCpusLinuxRefresher(CounterCpusLinux* counter):_counter(counter){
    ;
}

void CounterCpusLinuxRefresher::run(){
    double sleepingIntervalMs = (_counter->getWrappingInterval() / 2) * 1000;
    while(!_counter->_stopRefresher.timedWait(sleepingIntervalMs)){
        _counter->getJoulesComponentsAll();
    }
}

bool CounterCpusLinux::hasCoresCounter(topology::Cpu* cpu){	
  if(_family == CPU_FAMILY_INTEL){
    uint64_t dummy;
    return Msr(cpu->getVirtualCore()->getVirtualCoreId()).read(MSR_PP0_ENERGY_STATUS_INTEL, dummy) && dummy > 0;
  }else if(_family == CPU_FAMILY_AMD){
    return false; // TODO Return true and implement per-core readings
  }else{
    return false;
  }
}

bool CounterCpusLinux::hasGraphicCounter(topology::Cpu* cpu){	
  if(_family == CPU_FAMILY_INTEL){
    uint64_t dummy;
    return Msr(cpu->getVirtualCore()->getVirtualCoreId()).read(MSR_PP1_ENERGY_STATUS_INTEL, dummy) && dummy > 0;
  }else{
    return false;
  }
}

bool CounterCpusLinux::hasDramCounter(topology::Cpu* cpu){	
  if(_family == CPU_FAMILY_INTEL){
    uint64_t dummy;
    return Msr(cpu->getVirtualCore()->getVirtualCoreId()).read(MSR_DRAM_ENERGY_STATUS_INTEL, dummy) && dummy > 0;
  }else{
    return false;
  }
}

bool CounterCpusLinux::isCpuIntelSupported(topology::Cpu* cpu){
    Msr msr(cpu->getVirtualCore()->getVirtualCoreId());
    uint64_t dummy, dummy2, dummy3; // 3 different variables to avoid cppcheck warnings.
    return !cpu->getFamily().compare("6") &&
           !cpu->getVendorId().compare(0, 12, "GenuineIntel") &&
           msr.available() &&
           msr.read(MSR_RAPL_POWER_UNIT_INTEL, dummy) && dummy &&
           msr.read(MSR_PKG_POWER_INFO_INTEL, dummy2) && dummy2 &&
           msr.read(MSR_PKG_ENERGY_STATUS_INTEL, dummy3);
}

bool CounterCpusLinux::isCpuAMDSupported(topology::Cpu* cpu){
    Msr msr(cpu->getVirtualCore()->getVirtualCoreId());
    uint64_t dummy, dummy3; // 3 different variables to avoid cppcheck warnings.
    return !cpu->getFamily().compare("23") &&
           !cpu->getVendorId().compare(0, 12, "AuthenticAMD") &&
           msr.available() &&
           msr.read(MSR_RAPL_POWER_UNIT_AMD, dummy) && dummy &&
           msr.read(MSR_PKG_ENERGY_STATUS_AMD, dummy3);
}

bool CounterCpusLinux::isCpuSupported(topology::Cpu* cpu){
    return isCpuIntelSupported(cpu) || isCpuAMDSupported(cpu);
}

CounterCpusLinux::CounterCpusLinux():
        CounterCpus(topology::Topology::getInstance()),
        _initialized(false),
        _lock(),
        _stopRefresher(),
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

bool CounterCpusLinux::init(){
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

CounterCpusLinux::~CounterCpusLinux(){
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

uint32_t CounterCpusLinux::readEnergyCounter(topology::CpuId cpuId, int which){
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

double CounterCpusLinux::getWrappingInterval(){
    if(_family == CPU_FAMILY_INTEL){
      return ((double) 0xFFFFFFFF) * _energyPerUnit / _thermalSpecPower;
    }else if(_family == CPU_FAMILY_AMD){
      return 10; // No idea on how to compute wrapping interval on AMD, just put it to 10 seconds to be safe.
    }else{
      return 0;
    }
}

static uint32_t deltaDiff(uint32_t c1, uint32_t c2){
    if(c2 > c1){
        return c2 - c1;
    }else{
        return (((uint32_t)0xFFFFFFFF) - c1) + (uint32_t)1 + c2;
    }
}

void CounterCpusLinux::updateCounter(topology::CpuId cpuId, double& joules, uint32_t& lastReadCounter, uint32_t counterType){
    uint32_t currentCounter = readEnergyCounter(cpuId, counterType);
    joules += ((double) deltaDiff(lastReadCounter, currentCounter)) * _energyPerUnit;
    lastReadCounter = currentCounter;
}

JoulesCpu CounterCpusLinux::getJoulesComponents(topology::CpuId cpuId){
	return JoulesCpu(getJoulesCpu(cpuId), getJoulesCores(cpuId), getJoulesGraphic(cpuId), getJoulesDram(cpuId));
}

Joules CounterCpusLinux::getJoulesCpu(topology::CpuId cpuId){
    ScopedLock sLock(_lock);
    if(_family == CPU_FAMILY_INTEL){
      updateCounter(cpuId, _joulesCpus[cpuId].cpu, _lastReadCountersCpu[cpuId], MSR_PKG_ENERGY_STATUS_INTEL);
    }else if(_family == CPU_FAMILY_AMD){
      updateCounter(cpuId, _joulesCpus[cpuId].cpu, _lastReadCountersCpu[cpuId], MSR_PKG_ENERGY_STATUS_AMD);
    }
    return _joulesCpus[cpuId].cpu;
}

Joules CounterCpusLinux::getJoulesCores(topology::CpuId cpuId){
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

Joules CounterCpusLinux::getJoulesGraphic(topology::CpuId cpuId){
    if(hasJoulesGraphic()){
        ScopedLock sLock(_lock);
        if(_family == CPU_FAMILY_INTEL){
          updateCounter(cpuId, _joulesCpus[cpuId].graphic, _lastReadCountersGraphic[cpuId], MSR_PP1_ENERGY_STATUS_INTEL);
          return _joulesCpus[cpuId].graphic;
        }
    }
    return 0;
}

Joules CounterCpusLinux::getJoulesDram(topology::CpuId cpuId){
    if(hasJoulesDram()){
        ScopedLock sLock(_lock);
        if(_family == CPU_FAMILY_INTEL){
          updateCounter(cpuId, _joulesCpus[cpuId].dram, _lastReadCountersDram[cpuId], MSR_DRAM_ENERGY_STATUS_INTEL);
          return _joulesCpus[cpuId].dram;
        }
    }
    return 0;
}

void CounterCpusLinux::reset(){
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

bool CounterCpusLinux::hasJoulesCores(){
    return _hasJoulesCores;
}

bool CounterCpusLinux::hasJoulesDram(){
    return _hasJoulesDram;
}

bool CounterCpusLinux::hasJoulesGraphic(){
    return _hasJoulesGraphic;
}

PowerCapperLinux::PowerCapperLinux(CounterType type):PowerCapper(type), _good(false){
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
}

PowerCapperLinux::~PowerCapperLinux(){
  if(_good){
     raplcap_destroy(&_rc);
  }
}

bool PowerCapperLinux::init(){
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
}

}
}
