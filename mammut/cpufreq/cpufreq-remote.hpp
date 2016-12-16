#ifndef MAMMUT_CPUFREQ_REMOTE_HPP_
#define MAMMUT_CPUFREQ_REMOTE_HPP_

#include "../cpufreq/cpufreq.hpp"

namespace mammut{
namespace cpufreq{

class DomainRemote: public Domain{
public:
    DomainRemote(Communicator* const communicator, DomainId domainIdentifier,
                 std::vector<topology::VirtualCore*> virtualCores);
    void removeTurboFrequencies();
    std::vector<Frequency> getAvailableFrequencies() const;
    std::vector<Governor> getAvailableGovernors() const;
    Frequency getCurrentFrequency() const;
    Frequency getCurrentFrequencyUserspace() const;
    bool setFrequencyUserspace(Frequency frequency) const;
    Governor getCurrentGovernor() const;
    bool setGovernor(Governor governor) const;
    void getHardwareFrequencyBounds(Frequency& lowerBound,
                                    Frequency& upperBound) const;
    bool getCurrentGovernorBounds(Frequency& lowerBound,
                                  Frequency& upperBound) const;
    bool setGovernorBounds(Frequency lowerBound, Frequency upperBound) const;
    int getTransitionLatency() const;
    Voltage getCurrentVoltage() const;
    VoltageTable getVoltageTable(bool onlyPhysicalCores) const{
        throw std::runtime_error("notsupported");
    }
    VoltageTable getVoltageTable(uint numVirtualCores,
                                 bool onlyPhysicalCores) const{
        throw std::runtime_error("notsupported");
    }
private:
    Frequency getCurrentFrequency(bool userspace) const;
    Communicator* const _communicator;
    std::vector<Frequency> _availableFrequencies;
};

class CpuFreqRemote: public CpuFreq{
private:
private:
    Communicator* const _communicator;
    std::vector<Domain*> _domains;
public:
    CpuFreqRemote(Communicator* const communicator);
    ~CpuFreqRemote();
    void removeTurboFrequencies();
    std::vector<Domain*> getDomains() const;
    bool isBoostingSupported() const;
    bool isBoostingEnabled() const;
    void enableBoosting() const;
    void disableBoosting() const;
};

}
}

#endif /* MAMMUT_CPUFREQ_REMOTE_HPP_ */
