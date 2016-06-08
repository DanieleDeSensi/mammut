#include "../cpufreq/cpufreq.hpp"
#include "../cpufreq/cpufreq-linux.hpp"

#include "algorithm"
#include "fstream"
#include "iostream"
#include "limits"
#include "sstream"
#include "stdexcept"
#include "string"
#include "unistd.h"
#include "fstream"

namespace mammut{
namespace cpufreq{

using namespace std;
using namespace utils;


DomainLinux::DomainLinux(DomainId domainIdentifier, vector<topology::VirtualCore*> virtualCores):
        Domain(domainIdentifier, virtualCores),
        _msr(virtualCores.at(0)->getVirtualCoreId()){
    /** Reads available frequecies. **/
    Frequency frequency;
    for(size_t i = 0; i < virtualCores.size(); i++){
        _paths.push_back("/sys/devices/system/cpu/cpu" + intToString(virtualCores.at(i)->getVirtualCoreId()) + "/cpufreq/");
    }

    if(existsFile(_paths.at(0) + "scaling_available_frequencies")){
        ifstream freqFile((_paths.at(0) + "scaling_available_frequencies").c_str());
        if(freqFile){
            while(freqFile >> frequency){
                _availableFrequencies.push_back(frequency);
            }
            freqFile.close();
        }else{
            throw runtime_error("Impossible to open scaling_available_frequencies file.");
        }
    }else if(existsFile(_paths.at(0) + "stats/time_in_state")){
        vector<string> out = readFile(_paths.at(0) + "stats/time_in_state");
        for(size_t i = 0; i < out.size(); i++){
            _availableFrequencies.push_back(stringToUint(split(out[i], ' ')[0]));
        }
    }

    if(!_availableFrequencies.size()){
        throw runtime_error("No frequencies found.");
    }

    sort(_availableFrequencies.begin(), _availableFrequencies.end());

    /**
     * Remove turbo frequency if present.
     */
    if(intToString(_availableFrequencies.back()).at(3) == '1'){
        _turboFrequency = _availableFrequencies.back();
        _availableFrequencies.pop_back();
    }

    /** Reads available governors. **/
    string governorName;
    Governor governor;
    ifstream govFile((_paths.at(0) + "scaling_available_governors").c_str());
    if(govFile){
        while(govFile >> governorName){
            governor = CpuFreq::getGovernorFromGovernorName(governorName);
            if(governor != GOVERNOR_NUM){
                _availableGovernors.push_back(governor);
            }
        }
        govFile.close();
    }else{
        throw runtime_error("Impossible to open scaling_available_governors file.");
    }
}

void DomainLinux::writeToDomainFiles(const string& what, const string& where) const{
    for(size_t i = 0; i < _paths.size(); i++){
        ofstream file;
        file.open((_paths.at(i) + where).c_str());
        if(file){
            file << what;
            if(file.fail()){
                throw runtime_error("Write to frequency domain files failed.");
            }
            file.close();
        }else{
            throw runtime_error("Write to frequency domain files failed.");
        }
    }
}

vector<Frequency> DomainLinux::getAvailableFrequencies() const{
    return _availableFrequencies;
}

vector<Governor> DomainLinux::getAvailableGovernors() const{
    return _availableGovernors;
}

Frequency DomainLinux::getCurrentFrequency() const{
    string fileName = _paths.at(0) + "scaling_cur_freq";
    return stringToInt(readFirstLineFromFile(fileName));
}

Frequency DomainLinux::getCurrentFrequencyUserspace() const{
    switch(getCurrentGovernor()){
        case GOVERNOR_USERSPACE:{
            string fileName = _paths.at(0) + "scaling_setspeed";
            return stringToInt(readFirstLineFromFile(fileName));
        }
        default:{
            return 0;
        }
    }
}

Governor DomainLinux::getCurrentGovernor() const{
    string fileName = _paths.at(0) + "scaling_governor";
    return CpuFreq::getGovernorFromGovernorName(readFirstLineFromFile(fileName));
}

bool DomainLinux::setFrequencyUserspace(Frequency frequency) const{
    switch(getCurrentGovernor()){
        case GOVERNOR_USERSPACE:{
            if(!utils::contains(_availableFrequencies, frequency)){
                return false;
            }
            writeToDomainFiles(intToString(frequency), "scaling_setspeed");
            return true;
        }
        default:{
            return false;
        }
    }
}

void DomainLinux::getHardwareFrequencyBounds(Frequency& lowerBound, Frequency& upperBound) const{
    lowerBound = stringToInt(readFirstLineFromFile(_paths.at(0) + "cpuinfo_min_freq"));
    upperBound = stringToInt(readFirstLineFromFile(_paths.at(0) + "cpuinfo_max_freq"));
}

bool DomainLinux::getCurrentGovernorBounds(Frequency& lowerBound, Frequency& upperBound) const{
    switch(getCurrentGovernor()){
        case GOVERNOR_ONDEMAND:
        case GOVERNOR_CONSERVATIVE:
        case GOVERNOR_PERFORMANCE:
        case GOVERNOR_POWERSAVE:{
            lowerBound = stringToInt(readFirstLineFromFile(_paths.at(0) + "scaling_min_freq"));
            upperBound = stringToInt(readFirstLineFromFile(_paths.at(0) + "scaling_max_freq"));
            return true;
        }
        default:{
            return false;
        }
    }
}

bool DomainLinux::setGovernorBounds(Frequency lowerBound, Frequency upperBound) const{
    switch(getCurrentGovernor()){
        case GOVERNOR_ONDEMAND:
        case GOVERNOR_CONSERVATIVE:
        case GOVERNOR_PERFORMANCE:
        case GOVERNOR_POWERSAVE:{
            if(!utils::contains(getAvailableFrequencies(), lowerBound) ||
               !utils::contains(getAvailableFrequencies(), upperBound) ||
               lowerBound > upperBound){
                 return false;
            }

            writeToDomainFiles(intToString(lowerBound), "scaling_min_freq");
            writeToDomainFiles(intToString(upperBound), "scaling_max_freq");
            return true;
        }
        default:{
            return false;
        }
    }
}

bool DomainLinux::setGovernor(Governor governor) const{
    if(!utils::contains(_availableGovernors, governor)){
        return false;
    }

    writeToDomainFiles(CpuFreq::getGovernorNameFromGovernor(governor), "scaling_governor");
    return true;
}

int DomainLinux::getTransitionLatency() const{
    if(existsFile(_paths.at(0) + "cpuinfo_transition_latency")){
        return stringToInt(readFirstLineFromFile(_paths.at(0) + "cpuinfo_transition_latency"));
    }else{
        return -1;
    }
}

Voltage DomainLinux::getCurrentVoltage() const{
    uint64_t r;
    if(_msr.available() && _msr.readBits(MSR_PERF_STATUS, 47, 32, r) && r){
        return (double)r / (double)(1 << 13);
    }else{
        return 0;
    }
}

VoltageTable DomainLinux::getVoltageTable(bool onlyPhysicalCores) const{
    VoltageTable r;
    size_t numCores = 0;
    if(onlyPhysicalCores){
        numCores = topology::getNumPhysicalCores(_virtualCores);
    }else{
        numCores = _virtualCores.size();
    }
    for(size_t i = 0; i <= numCores; i++){
        VoltageTable tmp = getVoltageTable((uint)i, onlyPhysicalCores);
        for(VoltageTableIterator iterator = tmp.begin(); iterator != tmp.end(); iterator++){
            r.insert(*iterator);
        }
    }
    return r;
}

VoltageTable DomainLinux::getVoltageTable(uint numVirtualCores, bool onlyPhysicalCores) const{
    const uint numSamples = 5;
    const uint sleepInterval = 3;
    VoltageTable r;
    RollbackPoint rp = getRollbackPoint();

    if(!setGovernor(GOVERNOR_USERSPACE)){
        return r;
    }

    vector<topology::VirtualCore*> vcToMax;
    if(onlyPhysicalCores){
        vcToMax = getOneVirtualPerPhysical(_virtualCores);
    }else{
        vcToMax = _virtualCores;
    }

    if(numVirtualCores > vcToMax.size()){
        numVirtualCores = vcToMax.size();
    }

    for(size_t i = 0; i < numVirtualCores; i++){
        vcToMax.at(i)->maximizeUtilization();
    }

    for(size_t i = 0; i < _availableFrequencies.size(); i++){
        setFrequencyUserspace(_availableFrequencies.at(i));
        Voltage voltageSum = 0;
        for(uint j = 0; j < numSamples; j++){
            sleep(sleepInterval);
            voltageSum += getCurrentVoltage();
        }

        VoltageTableKey key(numVirtualCores, _availableFrequencies.at(i));
        r.insert(pair<VoltageTableKey, Voltage>(key, (voltageSum / (double)numSamples)));
    }

    for(size_t i = 0; i < numVirtualCores; i++){
        vcToMax.at(i)->resetUtilization();
    }

    rollback(rp);
    return r;
}

CpuFreqLinux::CpuFreqLinux():
    _boostingFile("/sys/devices/system/cpu/cpufreq/boost"){
    if(existsDirectory("/sys/devices/system/cpu/cpu0/cpufreq")){
        _topology = topology::Topology::local();
        vector<string> output;

        /** If freqdomain_cpus file are present, we must consider them instead of related_cpus. **/
        string domainsFiles;
        if(existsFile("/sys/devices/system/cpu/cpu0/cpufreq/freqdomain_cpus")){
            domainsFiles = "freqdomain_cpus";
        }else{
            domainsFiles = "related_cpus";
        }

        output = getCommandOutput("cat /sys/devices/system/cpu/cpu*/cpufreq/" + domainsFiles + " | sort | uniq");

        vector<topology::VirtualCore*> vc = _topology->getVirtualCores();

        _domains.resize(output.size());
        for(size_t i = 0; i < output.size(); i++){
            /** Converts the line to a vector of virtual cores identifiers. **/
            stringstream ss(output.at(i).c_str());
            vector<topology::VirtualCoreId> virtualCoresIdentifiers;
            topology::VirtualCoreId num;
            while(ss >> num){
                virtualCoresIdentifiers.push_back(num);
            }
            /** Creates a domain based on the vector of cores identifiers. **/
            _domains.at(i) = new DomainLinux(i, filterVirtualCores(vc, virtualCoresIdentifiers));
        }
    }
}

CpuFreqLinux::~CpuFreqLinux(){
    deleteVectorElements<Domain*>(_domains);
    topology::Topology::release(_topology);
}

vector<Domain*> CpuFreqLinux::getDomains() const{
    return _domains;
}

bool CpuFreqLinux::isBoostingSupported() const{
    //TODO: Se esiste il file è abilitabile dinamicamente. Potrebbe esserci boosting anche se il file non esiste?
    return existsFile(_boostingFile);
}

bool CpuFreqLinux::isBoostingEnabled() const{
    if(isBoostingSupported()){
        if(stringToInt(readFirstLineFromFile(_boostingFile))){
            return true;
        }
    }
    return false;
}

void CpuFreqLinux::enableBoosting() const{
    writeFile(_boostingFile, "1");
}

void CpuFreqLinux::disableBoosting() const{
    writeFile(_boostingFile, "0");
}

}
}
