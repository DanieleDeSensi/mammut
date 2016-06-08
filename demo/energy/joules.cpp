#include <mammut/mammut.hpp>

#include <cassert>
#include <iostream>
#include <unistd.h>

using namespace mammut;
using namespace mammut::energy;
using namespace std;

int main(int argc, char** argv){
    Mammut m;
    Energy* energy = m.getInstanceEnergy();
    Joules j;

    /** Gets the energy counters (one per CPU). **/
    Counter* counter = energy->getCounter();
    if(!counter){
        cout << "Power counters not available on this machine." << endl;
        return -1;
    }

    counter->reset();
    sleep(2);
    j = counter->getJoules();
    cout << j << " joules consumed in the last 2 seconds." << endl;

    counter->reset();
    sleep(4);
    j = counter->getJoules();
    cout << j << " joules consumed in the last 4 seconds." << endl;
}
