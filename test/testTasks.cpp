/**
 *  Different tests on task module.
 **/
#include <algorithm>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include "../mammut/mammut.hpp"
#include "gtest/gtest.h"

using namespace mammut;
using namespace mammut::task;
using namespace std;

TEST(TaskTest, GeneralTest) {
    Mammut m;
    SimulationParameters p;
    p.sysfsRootPrefix = "./archs/repara/";
    m.setSimulationParameters(p);
    TasksManager* task = m.getInstanceTask();

    pid_t pid = 0;
    if((pid = fork())){
        ProcessHandler* ph = task->getProcessHandler(pid);
        mammut::topology::Topology* topo = m.getInstanceTopology();
        ph->move(topo->getVirtualCore(0));
        double coreUsage = 0;
        sleep(4);
        ph->getCoreUsage(coreUsage);
        //EXPECT_L
        std::cout << "Core usage: " << coreUsage << std::endl;
        ph->throttle(30.0);
        ph->resetCoreUsage();
        sleep(4);
        ph->getCoreUsage(coreUsage);
        std::cout << "Core usage: " << coreUsage << std::endl;
        ph->sendSignal(SIGKILL);
    }else{
        double x = 23.444;
        while(true){
            x = std::sin(x);
        }
        std::cout << "Dummy: " << x << std::endl;
    }
}
