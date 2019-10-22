#ifndef MAMMUT_CPUFREQ_LINUX_HPP_
#define MAMMUT_CPUFREQ_LINUX_HPP_

#include "../cpufreq/cpufreq.hpp"

#include "vector"
#include "map"

namespace mammut{
namespace cpufreq{

class DomainLinux: public Domain{
public:
    DomainLinux(DomainId domainIdentifier, std::vector<topology::VirtualCore*> virtualCores);
    void removeTurboFrequencies();
    void reinsertTurboFrequencies();
    std::vector<Frequency> getAvailableFrequencies() const;
    std::vector<Governor> getAvailableGovernors() const;
    Frequency getCurrentFrequency() const;
    Frequency getCurrentFrequencyUserspace() const;
    bool setFrequencyUserspace(Frequency frequency) const;
    Governor getCurrentGovernor() const;
    bool setGovernor(Governor governor) const;
    void getHardwareFrequencyBounds(Frequency& lowerBound, Frequency& upperBound) const;
    bool getCurrentGovernorBounds(Frequency& lowerBound, Frequency& upperBound) const;
    bool setGovernorBounds(Frequency lowerBound, Frequency upperBound) const;
    int getTransitionLatency() const;
    Voltage getCurrentVoltage() const;
    VoltageTable getVoltageTable(bool onlyPhysicalCores = true) const;
    VoltageTable getVoltageTable(uint numVirtualCores, bool onlyPhysicalCores) const;
private:
    std::vector<Governor> _availableGovernors;
    std::vector<Frequency> _availableFrequencies;
    std::vector<std::string> _paths;
    mutable utils::Msr _msr;
    std::vector<Frequency> _turboFrequencies;
    bool _epyc;

    void writeToDomainFiles(const std::string& what, const std::string& where) const;
};

class CpuFreqLinux: public CpuFreq{
private:
    std::vector<Domain*> _domains;
    std::string _boostingFile;
    topology::Topology* _topology;
public:
    CpuFreqLinux();
    ~CpuFreqLinux();
    std::vector<Domain*> getDomains() const;
    bool isBoostingSupported() const;
    bool isBoostingEnabled() const;
    void enableBoosting() const;
    void disableBoosting() const;
};

}
}






#endif /* MAMMUT_CPUFREQ_LINUX_HPP_ */
