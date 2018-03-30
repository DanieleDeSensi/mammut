/**
 *  Different tests on task module.
 **/
#include <algorithm>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include "../mammut/mammut.hpp"
#if defined (__linux__)
#include "../mammut/mammut/task/task-linux.hpp"
#endif
#include "gtest/gtest.h"

using namespace mammut;
using namespace mammut::task;
using namespace mammut::topology;
using namespace std;

TEST(TaskTest, MiscTest) {
    Mammut m;
    SimulationParameters p;
    p.sysfsRootPrefix = "./archs/repara/";
    m.setSimulationParameters(p);
    TasksManager* task = m.getInstanceTask();
    ProcessHandler* ph = task->getProcessHandler(getpid());
    EXPECT_EQ(ph->getId(), getpid());
#if defined (__linux__)
    EXPECT_EQ(dynamic_cast<ProcessHandlerLinux*>(ph)->getPath(), std::string("/proc/") + utils::intToString(getpid()) + std::string("/"));
#endif
    EXPECT_TRUE(ph->isActive());
    uint oldPriority = 0, newPriority = 0;
    ph->getPriority(oldPriority);
    ph->setPriority(MAMMUT_PROCESS_PRIORITY_MAX);
    ph->getPriority(newPriority);
    EXPECT_EQ(newPriority, oldPriority); // We expect priority to do not change since it was run without sudo

    Topology* topology = m.getInstanceTopology();
    vector<Cpu*> cpus = topology->getCpus();
    EXPECT_TRUE(ph->move(cpus[0]));
    VirtualCoreId virtualCoreId;
    ph->getVirtualCoreId(virtualCoreId);
    bool found = false;
    for(auto t : cpus[0]->getVirtualCores()){
        if(t->getVirtualCoreId() == virtualCoreId){
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);

    vector<PhysicalCore*> phyCores = topology->getPhysicalCores();
    EXPECT_TRUE(ph->move(phyCores[0]));
    ph->getVirtualCoreId(virtualCoreId);
    found = false;
    for(auto t : phyCores[0]->getVirtualCores()){
        if(t->getVirtualCoreId() == virtualCoreId){
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);

    ph->move((VirtualCoreId) 0);
    ph->getVirtualCoreId(virtualCoreId);
    EXPECT_EQ(virtualCoreId, (VirtualCoreId) 0);

    std::vector<TaskId> activeProcesses = task->getActiveProcessesIdentifiers();
    EXPECT_TRUE(utils::contains(activeProcesses, getpid()));

    std::vector<topology::VirtualCoreId> virtualCoresIds;
    virtualCoresIds.push_back(1);
    virtualCoresIds.push_back(2);
    ph->move(virtualCoresIds);
    ph->getVirtualCoreId(virtualCoreId);
    EXPECT_TRUE(virtualCoreId == 1 || virtualCoreId == 2);

    ThreadHandler* thHandler = ph->getThreadHandler(ph->getActiveThreadsIdentifiers()[0]);
    ph->releaseThreadHandler(thHandler);
    try{
        double instructions = 0;
        ph->getAndResetInstructions(instructions);
#ifndef WITH_PAPI
        EXPECT_TRUE(false);
#endif
    } catch (...) {
#ifdef WITH_PAPI
        EXPECT_TRUE(false);
#endif
    }

    try{
        ph->resetInstructions();
#ifndef WITH_PAPI
        EXPECT_TRUE(false);
#endif
    } catch (...) {
#ifdef WITH_PAPI
        EXPECT_TRUE(false);
#endif
    }
    task->releaseProcessHandler(ph);
}

TEST(TaskTest, ThrottlingTest) {
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
        for(size_t i = 5; i < 100; i += 10){
            ph->throttle(i);
            sleep(1); // To be sure the throttler thread received the request.
            ph->resetCoreUsage();
            sleep(2);
            ph->getCoreUsage(coreUsage);
            // We specified a throttling of i%. We expect 
            // the actual usage to be in the range [i - 1, i + 1]
            EXPECT_LE(coreUsage, i + 1);
            EXPECT_GE(coreUsage, i - 1);
        }
        ph->sendSignal(SIGKILL);
    }else{
        double x = 23.444;
        while(true){
            x = std::sin(x);
        }
        std::cout << "Dummy: " << x << std::endl;
    }
}
