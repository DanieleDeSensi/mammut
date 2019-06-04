#ifndef MAMMUT_ENERGY_REMOTE_HPP_
#define MAMMUT_ENERGY_REMOTE_HPP_

#include "../energy/energy.hpp"
#ifdef MAMMUT_REMOTE
#include "../energy/energy-remote.pb.h"
#endif

namespace mammut{
namespace energy{

class CounterPlugRemote: public CounterPlug{
    friend class Energy;
private:
    mammut::Communicator* const _communicator;
    bool init();
public:
    explicit CounterPlugRemote(mammut::Communicator* const communicator);
    Joules getJoules();
    void reset();
};

class CounterMemoryRemote: public CounterMemory{
    friend class Energy;
private:
    mammut::Communicator* const _communicator;
    bool init();
public:
    explicit CounterMemoryRemote(mammut::Communicator* const communicator);
    Joules getJoules();
    void reset();
};

class CounterCpusRemote: public CounterCpus{
    friend class Energy;
private:
    mammut::Communicator* const _communicator;
    bool _hasCores;
    bool _hasDram;
    bool _hasGraphic;
public:
    explicit CounterCpusRemote(mammut::Communicator* const communicator);

    JoulesCpu getJoulesComponents();
    JoulesCpu getJoulesComponents(topology::CpuId cpuId);
    Joules getJoulesCpu();
    Joules getJoulesCpu(topology::CpuId cpuId);
    Joules getJoulesCores();
    Joules getJoulesCores(topology::CpuId cpuId);
    Joules getJoulesGraphic();
    Joules getJoulesGraphic(topology::CpuId cpuId);
    Joules getJoulesDram();
    Joules getJoulesDram(topology::CpuId cpuId);
    bool hasJoulesCores();
    bool hasJoulesDram();
    bool hasJoulesGraphic();
    void reset();
private:
    bool init();
};

}
}



#endif /* MAMMUT_ENERGY_REMOTE_HPP_ */
