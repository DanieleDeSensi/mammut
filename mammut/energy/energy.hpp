#ifndef MAMMUT_ENERGY_HPP_
#define MAMMUT_ENERGY_HPP_

#include "../communicator.hpp"
#include "../module.hpp"
#include "../topology/topology.hpp"

namespace mammut{
namespace energy{

using Joules = double;
class JoulesCpu;

typedef enum{
    COUNTER_CPUS = 0,// Power measured at CPU level
    COUNTER_PLUG, // Power measured at the plug
}CounterType;

/*
 * ! \class Counter
 *   \brief A generic energy counter.
 *
 *   A generic energy counter.
 */
class Counter{
public:
    virtual ~Counter(){;}

    /**
     * Returns the joules consumed up to this moment.
     * @return The joules consumed up to this moment.
     */
    virtual Joules getJoules() = 0;

    /**
     * Resets the value of the counter.
     */
    virtual void reset() = 0;

    /**
     * Initializes the counter.
     * @return True if the counter is present, false otherwise.
     */
    virtual bool init() = 0;

    /**
     * Returns the type of this counter.
     * @return The type of this counter.
     */
    virtual CounterType getType() = 0;
};

/*
 * ! \class CounterPlug
 *   \brief A plug energy counter.
 *
 *   A plug energy counter.
 */
class CounterPlug: public Counter{
public:
    virtual Joules getJoules() = 0;
    virtual void reset() = 0;
    virtual bool init() = 0;
    CounterType getType(){return COUNTER_PLUG;}
};

/*
 * ! \class CounterCpus
 *   \brief A CPUs energy counter.
 *
 *   A CPUs energy counter.
 */
class CounterCpus: public Counter{
protected:
    topology::Topology* _topology;
    std::vector<topology::Cpu*> _cpus;
    explicit CounterCpus(topology::Topology* topology);
public:
    /**
     * Returns the Cpus.
     * @return The Cpus.
     */
    const std::vector<topology::Cpu*>& getCpus(){return _cpus;}

    /**
     * Returns the Joules consumed by a Cpu and its components
     * since the counter creation (or since the last call of reset()).
     * @param cpuId The identifier of a Cpu.
     * @return The Joules consumed by a Cpu and its components
     *         since the counter creation (or since the last call of reset()).
     */
    virtual JoulesCpu getJoulesComponents(topology::CpuId cpuId) = 0;

    /**
     * Returns the Joules consumed by a Cpu and its components
     * since the counter creation (or since the last call of reset()).
     * @param cpu The Cpu.
     * @return The Joules consumed by a Cpu and its components
     *         since the counter creation (or since the last call of reset()).
     */
    JoulesCpu getJoulesComponents(topology::Cpu* cpu);

    /**
     * Returns the Joules consumed by all the Cpus and their components
     * since the counter creation (or since the last call of reset()).
     * @return The Joules consumed by all the Cpus and their components
     *         since the counter creation (or since the last call of reset()).
     */
    virtual JoulesCpu getJoulesComponentsAll();

    /**
     * Returns the Joules consumed by a Cpu since the counter creation
     * (or since the last call of reset()).
     * @param cpuId The identifier of a Cpu.
     * @return The Joules consumed by a Cpu since the counter
     *         creation (or since the last call of reset()).
     */
    virtual Joules getJoulesCpu(topology::CpuId cpuId) = 0;

    /**
     * Returns the Joules consumed by a Cpu since the counter creation
     * (or since the last call of reset()).
     * @param cpu The Cpu.
     * @return The Joules consumed by a Cpu since the counter
     *         creation (or since the last call of reset()).
     */
    Joules getJoulesCpu(topology::Cpu* cpu);

    /**
     * Returns the Joules consumed by all the Cpus since the counter creation
     * (or since the last call of reset()).
     * @return The Joules consumed by all the Cpus since the counter
     *         creation (or since the last call of reset()).
     */
    virtual Joules getJoulesCpuAll();

    /**
     * Returns true if the counter for cores is present, false otherwise.
     * @return True if the counter for cores is present, false otherwise.
     */
    virtual bool hasJoulesCores() = 0;

    /**
     * Returns the Joules consumed by the cores of a Cpu since the counter creation
     * (or since the last call of reset()).
     * @param cpuId The identifier of a Cpu.
     * @return The Joules consumed by the cores of a Cpu since the counter
     *         creation (or since the last call of reset()).
     */
    virtual Joules getJoulesCores(topology::CpuId cpuId) = 0;

    /**
     * Returns the Joules consumed by the cores of a Cpu since the counter creation
     * (or since the last call of reset()).
     * @param cpu The Cpu.
     * @return The Joules consumed by the cores of a Cpu since the counter
     *         creation (or since the last call of reset()).
     */
    Joules getJoulesCores(topology::Cpu* cpu);

    /**
     * Returns the Joules consumed by the cores of all the Cpus since the counter creation
     * (or since the last call of reset()).
     * @return The Joules consumed by the cores of all the Cpus since the counter
     *         creation (or since the last call of reset()).
     */
    virtual Joules getJoulesCoresAll();

    /**
     * Returns true if the counter for integrated graphic card is present, false otherwise.
     * @return True if the counter for integrated graphic card is present, false otherwise.
     */
    virtual bool hasJoulesGraphic() = 0;

    /**
     * Returns the Joules consumed by a Cpu integrated graphic card (if present) since
     * the counter creation (or since the last call of reset()).
     * @param cpuId The identifier of a Cpu.
     * @return The Joules consumed by a Cpu integrated graphic card (if present)
     *         since the counter creation (or since the last call of reset()).
     */
    virtual Joules getJoulesGraphic(topology::CpuId cpuId) = 0;

    /**
     * Returns the Joules consumed by a Cpu integrated graphic card (if present) since
     * the counter creation (or since the last call of reset()).
     * @param cpu The Cpu.
     * @return The Joules consumed by a Cpu integrated graphic card (if present)
     *         since the counter creation (or since the last call of reset()).
     */
    Joules getJoulesGraphic(topology::Cpu* cpu);

    /**
     * Returns the Joules consumed by all the Cpus integrated graphic card (if present) since
     * the counter creation (or since the last call of reset()).
     * @return The Joules consumed by all the Cpus integrated graphic card (if present)
     *         since the counter creation (or since the last call of reset()).
     */
    virtual Joules getJoulesGraphicAll();

    /**
     * Returns true if the counter for DRAM is present, false otherwise.
     * @return True if the counter for DRAM is present, false otherwise.
     */
    virtual bool hasJoulesDram() = 0;

    /**
     * Returns the Joules consumed by a Cpu Dram since the counter creation
     * (or since the last call of reset()).
     * @param cpuId The identifier of a Cpu.
     * @return The Joules consumed by a Cpu Dram since the counter
     *         creation (or since the last call of reset()).
     */
    virtual Joules getJoulesDram(topology::CpuId cpuId) = 0;

    /**
     * Returns the Joules consumed by a Cpu Dram since the counter creation
     * (or since the last call of reset()).
     * @param cpu The Cpu.
     * @return The Joules consumed by a Cpu Dram since the counter
     *         creation (or since the last call of reset()).
     */
    Joules getJoulesDram(topology::Cpu* cpu);

    /**
     * Returns the Joules consumed by all the Cpus Dram since the counter creation
     * (or since the last call of reset()).
     * @return The Joules consumed by all the Cpus Dram since the counter
     *         creation (or since the last call of reset()).
     */
    virtual Joules getJoulesDramAll();

    Joules getJoules();
    virtual void reset() = 0;
    virtual bool init() = 0;
    virtual ~CounterCpus(){;}
    CounterType getType(){return COUNTER_CPUS;}
};

class Energy: public Module{
    MAMMUT_MODULE_DECL(Energy)
private:
    CounterPlug* _counterPlug;
    CounterCpus* _counterCpus;
    Energy();
    explicit Energy(Communicator* const communicator);
    ~Energy();
    bool processMessage(const std::string& messageIdIn, const std::string& messageIn,
                        std::string& messageIdOut, std::string& messageOut);
public:
    /**
     * Returns the most precise energy counter available on this machine, or
     * NULL if no energy counters are available.
     * @return The most precise energy counter available on this machine, or
     *         NULL if no energy counters are available.
     */
    Counter* getCounter() const;

    /**
     * Returns a vector of counters types available on this machine.
     * The counters are sorted from the most precise to the least precise
     * one. For example, if both CPUs counter and power plug counter are present,
     * the first element of the vector will be COUNTER_CPUS and the second
     * COUNTER_PLUG.
     * @return A vector of counters types available on this machine.
     */
    std::vector<CounterType> getCountersTypes() const;

    /**
     * Returns a counter of a specific type if present, NULL otherwise.
     * @param type The type of the counter.
     * @return A counter of the specified type if present, NULL otherwise.
     */
    Counter* getCounter(CounterType type) const;
};


/*!
 * \class JoulesCpu
 * \brief Represents the values that can be read from a Cpu energy counter.
 */
class JoulesCpu{
    friend std::ostream& operator<<(std::ostream& os, const JoulesCpu& obj);
public:
    Joules cpu;
    Joules cores;
    Joules graphic;
    Joules dram;

    JoulesCpu():cpu(0), cores(0), graphic(0), dram(0){;}

    JoulesCpu(Joules cpu, Joules cores, Joules graphic, Joules dram):
        cpu(cpu), cores(cores), graphic(graphic), dram(dram){;}

    /**
     * Zeroes its content.
     */
    void zero(){cpu = 0; cores = 0; graphic = 0; dram = 0;}

    void swap(JoulesCpu& x){
        using std::swap;

        swap(cpu, x.cpu);
        swap(cores, x.cores);
        swap(graphic, x.graphic);
        swap(dram, x.dram);
    }

    JoulesCpu& operator=(JoulesCpu rhs){
        swap(rhs);
        return *this;
    }

    JoulesCpu& operator+=(const JoulesCpu& rhs){
        cpu += rhs.cpu;
        cores += rhs.cores;
        graphic += rhs.graphic;
        dram += rhs.dram;
        return *this;
    }

    JoulesCpu& operator-=(const JoulesCpu& rhs){
        cpu -= rhs.cpu;
        cores -= rhs.cores;
        graphic -= rhs.graphic;
        dram -= rhs.dram;
        return *this;
    }

    JoulesCpu& operator*=(const JoulesCpu& rhs){
        cpu *= rhs.cpu;
        cores *= rhs.cores;
        graphic *= rhs.graphic;
        dram *= rhs.dram;
        return *this;
    }

    JoulesCpu& operator/=(const JoulesCpu& rhs){
        cpu /= rhs.cpu;
        cores /= rhs.cores;
        graphic /= rhs.graphic;
        dram /= rhs.dram;
        return *this;
    }

    JoulesCpu operator/=(double x){
        cpu /= x;
        cores /= x;
        graphic /= x;
        dram /= x;
        return *this;
    }

    JoulesCpu operator*=(double x){
        cpu *= x;
        cores *= x;
        graphic *= x;
        dram *= x;
        return *this;
    }
};

inline JoulesCpu operator+(const JoulesCpu& lhs, const JoulesCpu& rhs){
    JoulesCpu r = lhs;
    r += rhs;
    return r;
}

inline JoulesCpu operator-(const JoulesCpu& lhs, const JoulesCpu& rhs){
    JoulesCpu r = lhs;
    r -= rhs;
    return r;
}

inline JoulesCpu operator*(const JoulesCpu& lhs, const JoulesCpu& rhs){
    JoulesCpu r = lhs;
    r *= rhs;
    return r;
}

inline JoulesCpu operator/(const JoulesCpu& lhs, const JoulesCpu& rhs){
    JoulesCpu r = lhs;
    r /= rhs;
    return r;
}

inline JoulesCpu operator/(const JoulesCpu& lhs, double x){
    JoulesCpu r = lhs;
    r /= x;
    return r;
}

inline JoulesCpu operator*(const JoulesCpu& lhs, double x){
    JoulesCpu r = lhs;
    r *= x;
    return r;
}

inline std::ostream& operator<<(std::ostream& os, const JoulesCpu& obj){
    os << obj.cpu << "\t";
    os << obj.cores << "\t";
    os << obj.graphic << "\t";
    os << obj.dram << "\t";
    return os;
}

}
}

#endif /* MAMMUT_ENERGY_HPP_ */
