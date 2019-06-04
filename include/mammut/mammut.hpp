#ifndef MAMMUT_MAMMUT_HPP_
#define MAMMUT_MAMMUT_HPP_

#include <mammut/cpufreq/cpufreq.hpp>
#include <mammut/energy/energy.hpp>
#include <mammut/task/task.hpp>
#include <mammut/topology/topology.hpp>

namespace mammut{

class Mammut{
friend class Simulator;
private:
    mutable cpufreq::CpuFreq* _cpufreq;
    mutable energy::Energy* _energy;
    mutable topology::Topology* _topology;
    mutable task::TasksManager* _task;

    Communicator* _communicator;

    /**
     * Release all the modules instances.
     */
    void releaseModules();
public:
    //TODO: Rule of the three

    /**
     * Creates a mammut handler.
     * @param communicator If different from NULL, all the modules will interact with
     *        a remote machine specified in the communicator.
     */
    explicit Mammut(Communicator* const communicator = NULL);

    /**
     * Copy constructor.
     * @param other The object to copy from.
     */
    Mammut(const Mammut& other);

    /**
     * Destroyes this mammut instance.
     */
    ~Mammut();

    /**
     * Assigns an instance of mammut to another one.
     * @param other The other instance.
     */
    Mammut& operator=(const Mammut& other);

    /**
     * Returns an instance of the CpuFreq module.
     * @return An instance of the CpuFreq module.
     */
    cpufreq::CpuFreq* getInstanceCpuFreq() const;

    /**
     * Returns an instance of the Energy module.
     * @return An instance of the Energy module.
     */
    energy::Energy* getInstanceEnergy() const;

    /**
     * Returns an instance of the Task module.
     * @return An instance of the Task module.
     */
    task::TasksManager* getInstanceTask() const;

    /**
     * Returns an instance of the Topology module.
     * @return An instance of the Topology module.
     */
    topology::Topology* getInstanceTopology() const;

    /**
     * Sets the simulation parameters. Only intended for
     * testing purposes.
     * @param p The parameters.
     */
    void setSimulationParameters(SimulationParameters& p);
};

}



#endif /* MAMMUT_MAMMUT_HPP_ */
