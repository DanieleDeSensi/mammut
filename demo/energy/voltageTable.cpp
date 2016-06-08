#include <mammut/mammut.hpp>

#include <cassert>
#include <iostream>
#include <cmath>
#include <unistd.h>

using namespace mammut;
using namespace mammut::cpufreq;
using namespace std;

int main(int argc, char** argv){
    Mammut m;
    CpuFreq* frequency = m.getInstanceCpuFreq();

    Domain* domain = frequency->getDomains().at(0);
    cout << "Starting computing the voltage table..." << endl;
    VoltageTable vt = domain->getVoltageTable();
    dumpVoltageTable(vt, "voltageTable.txt");
    cout << "Voltage table computed and dumped on file" << endl;
}
