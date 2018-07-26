#include <mammut/mammut.hpp>

#include <cassert>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace mammut;
using namespace mammut::energy;
using namespace std;

int main(int argc, char** argv){
    Mammut m;
    Energy* energy = m.getInstanceEnergy();

    /** Gets the energy counters (one per CPU). **/
    CounterCpus* counterCpus = dynamic_cast<CounterCpus*>(energy->getCounter(COUNTER_CPUS));
    if(!counterCpus){
        cout << "Cpu counters not present on this machine." << endl;
        //return -1;
    }

    pid_t child;
    if ((child = fork())>0) {
      waitpid(child, NULL, 0);
      cout << "Total Cpus Joules: " << counterCpus->getJoulesCpuAll() << " ";
      if(counterCpus->hasJoulesCores()){
          cout << "Total Cores Joules: " << counterCpus->getJoulesCoresAll() << " ";
      }
      if(counterCpus->hasJoulesDram()){
          cout << "Total Dram Joules: " << counterCpus->getJoulesDramAll() << " ";
      }
      if(counterCpus->hasJoulesGraphic()){
          cout << "Total Graphic Joules: " << counterCpus->getJoulesGraphicAll() << " ";
      }
      cout << endl;
    }else if(!child){
        std::string cmd;
        for(int i = 1; i < argc; i++){
            cmd += argv[i];
            cmd += " ";
        }
        system(cmd.c_str());
    }
    return 0;
}
