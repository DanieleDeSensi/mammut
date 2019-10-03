#ifndef MAMMUT_ENERGY_LINUX_HPP_
#define MAMMUT_ENERGY_LINUX_HPP_

#include <mammut/energy/energy.hpp>
#include <mammut/topology/topology.hpp>
#include <raplcap/raplcap.h>

class SmartGauge; 

namespace mammut{
namespace energy{

class CounterAmesterLinux{
    friend class Energy;
    friend class CounterPlugAmesterLinux;
    friend class CounterMemoryAmesterLinux;
private:
    utils::AmesterSensor _sensorJoules, _sensorWatts;
    Joules _lastValue;
    ulong _lastTimestamp;
    long _timeOffset;

    Joules getAdjustedValue();
    bool init();
public:
    CounterAmesterLinux(std::string jlsSensor, std::string wtsSensor);
    Joules getJoules();
    void reset();
};

class CounterPlugSmartGaugeLinux: public CounterPlug{
    friend class Energy;
private:
    SmartGauge* _sg;
    Joules _lastValue;
    Joules getJoulesAbs();
    bool init();
public:
    CounterPlugSmartGaugeLinux(); 
    ~CounterPlugSmartGaugeLinux(); 
    Joules getJoules();
    void reset();
};

class CounterPlugAmesterLinux: public CounterPlug, CounterAmesterLinux{
    friend class Energy;
private:
    bool init(){return CounterAmesterLinux::init();}
public:
    Joules getJoules(){return CounterAmesterLinux::getJoules();}
    void reset(){CounterAmesterLinux::reset();}
    CounterPlugAmesterLinux():
        CounterAmesterLinux("JLS250US", "PWR250US"){;}
};

class CounterCpusLinux;

class CounterMemoryRaplLinux: public CounterMemory{
    friend class Energy;
private:
    CounterCpusLinux* _ccl;
    bool init();
public:
    explicit CounterMemoryRaplLinux();
    Joules getJoules();
    void reset();
};

class CounterMemoryAmesterLinux: public CounterMemory, CounterAmesterLinux{
    friend class Energy;
private:
    bool init(){return CounterAmesterLinux::init();}
public:
    Joules getJoules(){return CounterAmesterLinux::getJoules();}
    void reset(){CounterAmesterLinux::reset();}
    CounterMemoryAmesterLinux():
        CounterAmesterLinux("JLS250USMEM0", "PWR250USMEM0"){;}
};

class CounterCpusLinuxRefresher: public utils::Thread{
private:
    CounterCpusLinux* _counter;
public:
    explicit CounterCpusLinuxRefresher(CounterCpusLinux* counter);
    void run();
};

class CounterCpusLinux: public CounterCpus{
    friend class CounterCpusLinuxRefresher;
    friend class Energy;
    friend class CounterMemoryRaplLinux;
private:
    bool _initialized;
    utils::LockPthreadMutex _lock;
    utils::Monitor _stopRefresher;
    CounterCpusLinuxRefresher* _refresher;
    utils::Msr** _msrs;
    topology::CpuId _maxId;
    double _powerPerUnit;
    double _energyPerUnit;
    double _timePerUnit;
    double _thermalSpecPower;

    JoulesCpu* _joulesCpus;

    uint32_t* _lastReadCountersCpu;
    uint32_t* _lastReadCountersCores;
    uint32_t* _lastReadCountersGraphic;
    uint32_t* _lastReadCountersDram;

    bool _hasJoulesCores;
    bool _hasJoulesGraphic;
    bool _hasJoulesDram;

    uint32_t readEnergyCounter(topology::CpuId cpuId, int which);

    /**
     * Returns the minimum number of seconds needed by the counter to wrap.
     * @return The minimum number of seconds needed by the counter to wrap.
     */
    double getWrappingInterval();

    /**
     * Adds to the 'joules' counter the joules consumed from lastReadCounter to
     * the time of this call.
     * @param cpuId The identifier of the CPU.
     * @param joules The joules counter.
     * @param lastReadCounter The last value read for counter counterType.
     * @param counterType The type of the counter.
     */
    void updateCounter(topology::CpuId cpuId, Joules& joules, uint32_t& lastReadCounter, uint32_t counterType);

    static bool hasCoresCounter(topology::Cpu* cpu);
    static bool hasGraphicCounter(topology::Cpu* cpu);
    static bool hasDramCounter(topology::Cpu* cpu);
    static bool isCpuSupported(topology::Cpu* cpu);
public:
    CounterCpusLinux();

    JoulesCpu getJoulesComponents(topology::CpuId cpuId);
    Joules getJoulesCpu(topology::CpuId cpuId);
    Joules getJoulesCores(topology::CpuId cpuId);
    Joules getJoulesGraphic(topology::CpuId cpuId);
    Joules getJoulesDram(topology::CpuId cpuId);

    bool hasJoulesCores();
    bool hasJoulesDram();
    bool hasJoulesGraphic();    
    void reset();
private:
    bool init();
    ~CounterCpusLinux();
};

class PowerCapperLinux : PowerCapper{
  friend class Energy;
private:
  bool _good;
  raplcap _rc;
  size_t _sockets;
  raplcap_zone _zone;
  bool init();
public:
  PowerCapperLinux(CounterType type);
  ~PowerCapperLinux();
  std::vector<std::pair<PowerCap, PowerCap> > get() const;
  std::pair<PowerCap, PowerCap> get(uint socketId) const;
  PowerCap get(uint socketId, uint windowId) const;
  void set(PowerCap cap);
  void set(uint socketId, PowerCap cap);
  void set(uint windowId, uint socketId, PowerCap cap);
};

}
}

#endif /* MAMMUT_ENERGY_LINUX_HPP_ */
