#include <mammut/cpufreq/cpufreq.hpp>
#include <mammut/cpufreq/cpufreq-linux.hpp>

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
extern SimulationParameters simulationParameters;

namespace cpufreq{

using namespace std;
using namespace utils;


DomainLinux::DomainLinux(DomainId domainIdentifier, vector<topology::VirtualCore*> virtualCores):
        Domain(domainIdentifier, virtualCores),
        _msr(virtualCores.at(0)->getVirtualCoreId(), O_RDWR){

    topology::Topology* top = topology::Topology::getInstance();
    std::vector<topology::Cpu*> cpus = top->getCpus();
    if(!cpus[0]->getFamily().compare("23") &&
       !cpus[0]->getVendorId().compare(0, 12, "AuthenticAMD")){
      _epyc = true;
      for(int i = 8; i >= 0; i--){
        uint64_t fId = 0, dfsId = 0;
        bool fIdr, dfsIdr;
        fIdr = _msr.readBits(0xC0010064 + i, 7, 0, fId);
        dfsIdr = _msr.readBits(0xC0010064 + i, 13, 8, dfsId);
        if(!fIdr || !dfsIdr || !fId || !dfsId){
          continue;
        }else{
	  _availableFrequencies.push_back((fId / dfsId)*200.0*1000); // *1000 -> KHz to Hz
	}
      }
      _availableGovernors.push_back(GOVERNOR_USERSPACE);
    }else{
      _epyc = false;
      /** Reads available frequecies. **/
      for(size_t i = 0; i < virtualCores.size(); i++){
          _paths.push_back(simulationParameters.sysfsRootPrefix +
                           "/sys/devices/system/cpu/cpu" +
                           intToString(virtualCores.at(i)->getVirtualCoreId()) +
                           "/cpufreq/");
      }

      if(existsFile(_paths.at(0) + "scaling_available_frequencies")){
          ifstream freqFile((_paths.at(0) + "scaling_available_frequencies").c_str());
          if(freqFile){
              Frequency frequency;
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

      if(_availableFrequencies.size()){
          sort(_availableFrequencies.begin(), _availableFrequencies.end());
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

void DomainLinux::removeTurboFrequencies(){
#if defined(__x86_64__) // It seems that on Power8 this is not the case
    if(_turboFrequencies.empty()){
        for(auto it = _availableFrequencies.begin(); it != _availableFrequencies.end();){
            if(intToString(*it).at(3) == '1'){
                _turboFrequencies.push_back(*it);
                it = _availableFrequencies.erase(it);
            }else{
                ++it;
            }
        }
    }
#endif
}

void DomainLinux::reinsertTurboFrequencies(){
    for(Frequency f : _turboFrequencies){
        _availableFrequencies.push_back(f);
    }
    _turboFrequencies.clear();
}

vector<Frequency> DomainLinux::getAvailableFrequencies() const{
    return _availableFrequencies;
}

vector<Governor> DomainLinux::getAvailableGovernors() const{
    return _availableGovernors;
}

Frequency DomainLinux::getCurrentFrequency() const{
    if(_epyc){
      return 0; // TODO
    }else{
      string fileName = _paths.at(0) + "scaling_cur_freq";
      return stringToInt(readFirstLineFromFile(fileName));
    }
}

static uint64_t getCurrentEpycPstate(const Msr& msr){
  uint64_t pState;
  msr.readBits(0xC0010062, 2, 0, pState);
  return pState;
}

Frequency DomainLinux::getCurrentFrequencyUserspace() const{
    if(_epyc){
      return _availableFrequencies[_availableFrequencies.size() - 1 - getCurrentEpycPstate(_msr)];
    }else{
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
}

Governor DomainLinux::getCurrentGovernor() const{
    if(_epyc){
      return GOVERNOR_USERSPACE;
    }else{
      string fileName = _paths.at(0) + "scaling_governor";
      return CpuFreq::getGovernorFromGovernorName(readFirstLineFromFile(fileName));
    }
}

bool DomainLinux::setFrequencyUserspace(Frequency frequency) const{
    if(_epyc){
      uint64_t pState;
      bool found = false;
      for(size_t i = 0; i < _availableFrequencies.size(); i++){
        if(frequency == _availableFrequencies[i]){
          pState = _availableFrequencies.size() - 1 - i;
          found = true;
          break;
        }
      }
      if(found){
        return _msr.writeBits(0xC0010062, 2, 0, pState);
      }else{
        return false;
      }
    }else{
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
}

void DomainLinux::getHardwareFrequencyBounds(Frequency& lowerBound, Frequency& upperBound) const{
    if(_epyc){
      lowerBound = 0;
      upperBound = 0;
    }else{
      lowerBound = stringToInt(readFirstLineFromFile(_paths.at(0) + "cpuinfo_min_freq"));
      upperBound = stringToInt(readFirstLineFromFile(_paths.at(0) + "cpuinfo_max_freq"));
    }
}

bool DomainLinux::getCurrentGovernorBounds(Frequency& lowerBound, Frequency& upperBound) const{
    if(_epyc){
      return false;
    }else{
      lowerBound = stringToInt(readFirstLineFromFile(_paths.at(0) + "scaling_min_freq"));
      upperBound = stringToInt(readFirstLineFromFile(_paths.at(0) + "scaling_max_freq"));
      return true;
    }
}

bool DomainLinux::setGovernorBounds(Frequency lowerBound, Frequency upperBound) const{
    if(_epyc){
      return false;
    }else{
      if(!utils::contains(getAvailableFrequencies(), lowerBound) ||
         !utils::contains(getAvailableFrequencies(), upperBound) ||
         lowerBound > upperBound){
           return false;
      }

      writeToDomainFiles(intToString(lowerBound), "scaling_min_freq");
      writeToDomainFiles(intToString(upperBound), "scaling_max_freq");
      return true;
    }
}

bool DomainLinux::setGovernor(Governor governor) const{
    if(_epyc){
      if(governor == GOVERNOR_USERSPACE){
        return true;
      }else{
        return false;
      }
    }else{
      if(!utils::contains(_availableGovernors, governor)){
          return false;
      }

      writeToDomainFiles(CpuFreq::getGovernorNameFromGovernor(governor), "scaling_governor");
      return true;
    }
}

int DomainLinux::getTransitionLatency() const{
    if(!_paths.empty() && existsFile(_paths.at(0) + "cpuinfo_transition_latency")){
        return stringToInt(readFirstLineFromFile(_paths.at(0) + "cpuinfo_transition_latency"));
    }else{
        return -1;
    }
}

Voltage DomainLinux::getCurrentVoltage() const{
    if(_epyc){
      uint64_t pState = getCurrentEpycPstate(_msr);
      uint64_t vid;
      if(_msr.readBits(0xC0010064 + pState, 21, 14, vid)){
        return 1.550 - 0.00625*vid;
      }else{
        return 0;
      }
    }else{
      uint64_t r;
      if(_msr.available() && _msr.readBits(MSR_PERF_STATUS, 47, 32, r) && r){
        // Intel
          return (double)r / (double)(1 << 13);
      }else{
        // Power8 (AMESTER)
        AmesterSensor voltage("VOLT250USP0V0");
        if(voltage.exists()){
          return voltage.readSum().value;
        }
        // Unsupported
        return 0;
      }
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
        for(VoltageTableIterator iterator = tmp.begin(); iterator != tmp.end(); ++iterator){
            r.insert(*iterator);
        }
    }
    return r;
}

VoltageTable DomainLinux::getVoltageTable(uint numVirtualCores, bool onlyPhysicalCores) const{
    uint numSamples;
    uint sleepInterval;
    if(_epyc){
      numSamples = 1;
      sleepInterval = 0;
    }else{
      numSamples = 5;
      sleepInterval = 3;
    }
    VoltageTable r;
    
    Governor oldGovernor = getCurrentGovernor();
    Frequency oldFrequency = getCurrentFrequencyUserspace();
    Frequency oldFrequencyLb, oldFrequencyUb;
    if(!_epyc){
      getCurrentGovernorBounds(oldFrequencyLb, oldFrequencyUb);
    }

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

    setGovernor(oldGovernor);
    if(oldGovernor == GOVERNOR_USERSPACE){
        setFrequencyUserspace(oldFrequency);
    }else{
        if(!_epyc){
          setGovernorBounds(oldFrequencyLb, oldFrequencyUb);
        }
    }
    return r;
}

CpuFreqLinux::CpuFreqLinux():
    _boostingFile(simulationParameters.sysfsRootPrefix +
                  "/sys/devices/system/cpu/cpufreq/boost"){
    if(existsDirectory(simulationParameters.sysfsRootPrefix +
                       "/sys/devices/system/cpu/cpu0/cpufreq")){
        _topology = topology::Topology::local();
        vector<string> output;

        /** If freqdomain_cpus file are present, we must consider them instead of related_cpus. **/
        string domainsFiles;
        if(existsFile(simulationParameters.sysfsRootPrefix +
                      "/sys/devices/system/cpu/cpu0/cpufreq/freqdomain_cpus")){
            domainsFiles = "freqdomain_cpus";
        }else{
            domainsFiles = "related_cpus";
        }

        output = getCommandOutput("cat " + simulationParameters.sysfsRootPrefix
                                  + "/sys/devices/system/cpu/cpu*/cpufreq/" +
                                  domainsFiles + " | sort | uniq");

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
    }else{
      topology::Topology* top = topology::Topology::getInstance();
      std::vector<topology::Cpu*> cpus = top->getCpus();
      if(!cpus[0]->getFamily().compare("23") &&
         !cpus[0]->getVendorId().compare(0, 12, "AuthenticAMD")){
        std::vector<topology::PhysicalCore*> cores = top->getPhysicalCores();
        size_t i = 0;
        for(auto c : cores){
          _domains.push_back(new DomainLinux(i, c->getVirtualCores()));
          i++;
        }
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
