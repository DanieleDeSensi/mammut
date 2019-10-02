#include <mammut/mammut.hpp>

#include <cassert>
#include <chrono>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std::chrono;

using namespace mammut;
using namespace mammut::energy;
using namespace std;

static bool isRunning(pid_t pid) {
  return !waitpid(pid, NULL, WNOHANG);
}

static std::string counterTypeToString(CounterType ct){
  switch(ct){
  case COUNTER_PLUG:{
    return "PLUG";
  }break;
  case COUNTER_CPUS:{
    return "CPUS";
  }break;
  case COUNTER_MEMORY:{
    return "MEMORY";
  }break;
  default:{
    return "UNKNOWN";
  }
  }
}

static double samplingInterval = 1.0;

int main(int argc, char** argv){
  pid_t child;
  if ((child = fork())>0) {
    auto startTime = duration_cast< milliseconds >(
          system_clock::now().time_since_epoch()
          ).count();

    Mammut m;
    Energy* energy = m.getInstanceEnergy();
    std::string outFilePrefix = "launcher_out";

    ofstream outFile, outFileSummary;
    outFile.open(outFilePrefix + ".csv");
    outFileSummary.open(outFilePrefix + "_summary.csv");

    outFile << "Time(s)\t";
    std::vector<Counter*> counters;
    std::vector<Joules> summary;
    for(int i = COUNTER_CPUS; i <= COUNTER_PLUG; i++){
      CounterType ct = static_cast<CounterType>(i);
      counters.push_back(energy->getCounter(ct));
      summary.push_back(0);
      if(counters.back()){
        outFile << counterTypeToString(ct) << "\t";
        outFileSummary << counterTypeToString(ct) << "\t";
      }
    }
    outFile << std::endl;
    outFileSummary << std::endl;

    while(isRunning(child)){
      sleep(samplingInterval);

      auto now = duration_cast< milliseconds >(
            system_clock::now().time_since_epoch()
            ).count();

      outFile << std::to_string(((now - startTime) / 1000.0)) << "\t";

      for(size_t i = 0; i < counters.size(); i++){
        Counter* c = counters[i];
        if(c){
          Joules j = c->getJoules() / samplingInterval;
          outFile << j << "\t";
          c->reset();
          summary[i] += j;
        }
      }
      outFile << std::endl;
    }

    for(size_t i = 0; i < summary.size(); i++){
      Joules j = summary[i];
      if(j){
        outFileSummary << j << "\t";
      }
    }
    outFileSummary << std::endl;

    outFile.close();
    outFileSummary.close();
  }else if(!child){
    std::string cmd;
    for(int i = 1; i < argc; i++){
      cmd += argv[i];
      cmd += " ";
    }
    int status = system(cmd.c_str());
    std::cout << "Application terminated with status: " << status << std::endl;
  }
  return 0;
}
