/**
 *  Different tests on topology module.
 **/
#include <algorithm>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include "../mammut/mammut.hpp"
#include "gtest/gtest.h"

using namespace mammut;
using namespace mammut::topology;
using namespace std;

// TODO: Only works for repara. Let it be parametric.
TEST(TopologyTest, GeneralTest) {
    Mammut m;
    SimulationParameters p;
    p.sysfsRootPrefix = "./archs/repara/";
    m.setSimulationParameters(p);
    Topology* topology = m.getInstanceTopology();
    vector<Cpu*> cpus = topology->getCpus();
    EXPECT_EQ(cpus.size(), (size_t) 2);
    for(size_t i = 0; i < cpus.size(); i++){
        Cpu* cpu = cpus.at(i);
        EXPECT_EQ(cpu->getCpuId(), i);
        EXPECT_STREQ(cpu->getVendorId().c_str(), "GenuineIntel");
        EXPECT_STREQ(cpu->getFamily().c_str(), "6");
        EXPECT_STREQ(cpu->getModel().c_str(), "62");
        EXPECT_EQ(cpu->getPhysicalCores().size(), (size_t) 12);
        EXPECT_EQ(cpu->getVirtualCores().size(), (size_t) 24);
    }

    vector<VirtualCore*> virtualCores = topology->getVirtualCores();

    for(size_t i = 0; i < virtualCores.size(); i++){
        VirtualCore* vc = virtualCores.at(i);
        EXPECT_TRUE(vc->isHotPluggable());
        EXPECT_TRUE(vc->isHotPlugged());
        EXPECT_TRUE(vc->areTicksConstant());
    }

    /*******************************************/
    /*              HotPlug test               */
    /*******************************************/
    virtualCores.back()->hotUnplug();
    EXPECT_FALSE(virtualCores.back()->isHotPlugged());
    virtualCores.back()->hotPlug();
    EXPECT_TRUE(virtualCores.back()->isHotPlugged());

    /*******************************************/
    /*              Utilisation test           */
    /*******************************************/
    for(size_t i = 0; i < virtualCores.size(); i++){
        virtualCores.at(i)->resetIdleTime();
    }
    vector<VirtualCoreIdleLevel*> idleLevels = virtualCores.at(0)->getIdleLevels();
    if(idleLevels.size()){
        for(size_t i = 0; i < idleLevels.size(); i++){
            idleLevels.at(i)->resetTime();
            idleLevels.at(i)->resetCount();
        }
    }

    uint sleepingSecs = 10;
    virtualCores.at(0)->maximizeUtilization();
    sleep(sleepingSecs);
    virtualCores.at(0)->resetUtilization();

#if 0
    for(size_t i = 0; i < virtualCores.size(); i++){
        VirtualCore* tmp = virtualCores.at(i);
        double idleTime = ((((double)tmp->getIdleTime())/1000000.0) / ((double)sleepingSecs)) * 100.0;
        EXPECT_LT(abs(idleTime - 100), 0.5);
    }
#endif

    if(idleLevels.size()){
        uint totalTime = 0;
        for(size_t i = 0; i < idleLevels.size(); i++){
            VirtualCoreIdleLevel* level = idleLevels.at(i);
            EXPECT_TRUE(level->isEnableable());
            EXPECT_TRUE(level->isEnabled());
            uint time = level->getTime();
            uint count = level->getCount();
            if(time == 0){
                EXPECT_EQ(count, (uint) 0);
            }
            if(count == 0){
                EXPECT_EQ(time, (uint) 0);
            }
            switch(level->getLevelId()){
            case 0:{
                EXPECT_STREQ(level->getName().c_str(), "POLL");
                EXPECT_STREQ(level->getDesc().c_str(), "CPUIDLE CORE POLL IDLE");
                EXPECT_EQ(level->getConsumedPower(), 4294967295);
                EXPECT_EQ(level->getExitLatency(), 0);
            }break;
            case 1:{
                EXPECT_STREQ(level->getName().c_str(), "C1-IVB");
                EXPECT_STREQ(level->getDesc().c_str(), "MWAIT 0x00");
                EXPECT_EQ(level->getConsumedPower(), 0);
                EXPECT_EQ(level->getExitLatency(), 1);
            }break;
            case 2:{
                EXPECT_STREQ(level->getName().c_str(), "C1E-IVB");
                EXPECT_STREQ(level->getDesc().c_str(), "MWAIT 0x01");
                EXPECT_EQ(level->getConsumedPower(), 0);
                EXPECT_EQ(level->getExitLatency(), 10);
            }break;
            case 3:{
                EXPECT_STREQ(level->getName().c_str(), "C3-IVB");
                EXPECT_STREQ(level->getDesc().c_str(), "MWAIT 0x10");
                EXPECT_EQ(level->getConsumedPower(), 0);
                EXPECT_EQ(level->getExitLatency(), 59);
            }break;
            case 4:{
                EXPECT_STREQ(level->getName().c_str(), "C6-IVB");
                EXPECT_STREQ(level->getDesc().c_str(), "MWAIT 0x20");
                EXPECT_EQ(level->getConsumedPower(), 0);
                EXPECT_EQ(level->getExitLatency(), 80);
            }break;
            default:{
                EXPECT_STREQ("Too many idle levels.", "");
            }break;
            }

            totalTime += time;
        }
        EXPECT_LT((double)virtualCores.at(idleLevels.at(0)->getVirtualCoreId())->getIdleTime() / 1000000.0, 0.1);
        EXPECT_LT(totalTime / 1000000.0, 0.1);
        EXPECT_GT(sleepingSecs - (totalTime / 1000000.0), 9.99);
    }
}
