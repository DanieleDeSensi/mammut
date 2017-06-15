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
using namespace mammut::cpufreq;
using namespace std;

// TODO: Only works for repara. Let it be parametric.
TEST(CpufreqTest, GeneralTest) {
    Mammut m;
    SimulationParameters p;
    p.sysfsRootPrefix = "./archs/repara/";
    m.setSimulationParameters(p);
    CpuFreq* frequency = m.getInstanceCpuFreq();
    EXPECT_TRUE(frequency->isBoostingSupported());
    EXPECT_TRUE(frequency->isBoostingEnabled());
    frequency->disableBoosting();
    EXPECT_TRUE(!frequency->isBoostingEnabled());
    frequency->enableBoosting();
    EXPECT_TRUE(frequency->isBoostingEnabled());

    /** Analyzing domains. **/
    std::vector<Domain*> domains = frequency->getDomains();
    EXPECT_EQ(domains.size(), (size_t) 2);
    static const int arr[] = {0, 24, 1, 25, 2, 26, 3, 27, 4, 28, 5, 29, 6, 30, 7, 31, 8, 32, 9, 33, 10, 34, 11, 35};
    vector<topology::VirtualCoreId> firstDomainCores(arr, arr + sizeof(arr)/sizeof(arr[0]));
    static const int arr2[] = {12, 36, 13, 37, 14, 38, 15, 39, 16, 40, 17, 41, 18, 42, 19, 43, 20, 44, 21, 45, 22, 46, 23, 47};
    vector<topology::VirtualCoreId> secondDomainCores(arr2, arr2 + sizeof(arr2)/sizeof(arr2[0]));
    static const int arr3[] = {1200000, 1300000, 1400000, 1500000, 1600000, 1700000,
                               1800000, 1900000, 2000000, 2100000, 2200000, 2300000,
                               2400000, 2401000};
    vector<Frequency> expectedFrequencies(arr3, arr3 + sizeof(arr3)/sizeof(arr3[0]));

    for(size_t i = 0; i < domains.size(); i++){
        Domain* domain = domains.at(i);
        std::vector<topology::VirtualCoreId> identifiers = domain->getVirtualCoresIdentifiers();
        EXPECT_EQ(domain->getId(), i);
        if(i == 0){
            // Check that they are equal. Done in this way to also test
            // the contains function.
            EXPECT_TRUE(utils::contains(identifiers, firstDomainCores));
            EXPECT_TRUE(utils::contains(firstDomainCores, identifiers));
        }else{
            EXPECT_TRUE(utils::contains(identifiers, secondDomainCores));
            EXPECT_TRUE(utils::contains(secondDomainCores, identifiers));
        }
        std::vector<Governor> governors = domain->getAvailableGovernors();
        EXPECT_TRUE(std::find(governors.begin(), governors.end(), GOVERNOR_CONSERVATIVE) != governors.end());
        EXPECT_TRUE(std::find(governors.begin(), governors.end(), GOVERNOR_USERSPACE) != governors.end());
        EXPECT_TRUE(std::find(governors.begin(), governors.end(), GOVERNOR_POWERSAVE) != governors.end());
        EXPECT_TRUE(std::find(governors.begin(), governors.end(), GOVERNOR_ONDEMAND) != governors.end());
        EXPECT_TRUE(std::find(governors.begin(), governors.end(), GOVERNOR_PERFORMANCE) != governors.end());
        EXPECT_STREQ(frequency->getGovernorNameFromGovernor(GOVERNOR_CONSERVATIVE).c_str(), "conservative");
        EXPECT_STREQ(frequency->getGovernorNameFromGovernor(GOVERNOR_USERSPACE).c_str(), "userspace");
        EXPECT_STREQ(frequency->getGovernorNameFromGovernor(GOVERNOR_POWERSAVE).c_str(), "powersave");
        EXPECT_STREQ(frequency->getGovernorNameFromGovernor(GOVERNOR_ONDEMAND).c_str(), "ondemand");
        EXPECT_STREQ(frequency->getGovernorNameFromGovernor(GOVERNOR_PERFORMANCE).c_str(), "performance");
        EXPECT_STREQ(frequency->getGovernorNameFromGovernor(GOVERNOR_INTERACTIVE).c_str(), "interactive");

        std::vector<Frequency> frequencies = domain->getAvailableFrequencies();
        EXPECT_TRUE(utils::contains(frequencies, expectedFrequencies));
        EXPECT_TRUE(utils::contains(expectedFrequencies, frequencies));
        EXPECT_EQ(domain->getTransitionLatency(), 10000);

        Governor currentGovernor = domain->getCurrentGovernor();
        EXPECT_EQ(currentGovernor, GOVERNOR_PERFORMANCE);

        Frequency lb, ub, currentFrequency;
        domain->getHardwareFrequencyBounds(lb, ub);
        EXPECT_EQ(lb, (Frequency) 1200000);
        EXPECT_EQ(ub, (Frequency) 2401000);

        if(currentGovernor == GOVERNOR_USERSPACE){
            currentFrequency = domain->getCurrentFrequencyUserspace();
        }else{
            currentFrequency = domain->getCurrentFrequency();
            EXPECT_TRUE(domain->getCurrentGovernorBounds(lb, ub));
            EXPECT_EQ(lb, (Frequency) 1200000);
            EXPECT_EQ(ub, (Frequency) 2401000);
        }
        EXPECT_EQ(currentFrequency, (Frequency) 2401000);

        /** Change frequency test. **/
        domain->setGovernor(GOVERNOR_USERSPACE);
        EXPECT_EQ(domain->getCurrentGovernor(), GOVERNOR_USERSPACE);
        domain->setFrequencyUserspace(frequencies.at(0));
        EXPECT_EQ(domain->getCurrentFrequencyUserspace(), frequencies.at(0));
        domain->setHighestFrequencyUserspace();
        domain->setFrequencyUserspace(frequencies.back());
        EXPECT_EQ(domain->getCurrentFrequencyUserspace(), frequencies.at(frequencies.size() - 1));
        /** Restore original governor and frequency. **/
        domain->setGovernor(currentGovernor);
        EXPECT_EQ(domain->getCurrentGovernor(), currentGovernor);
        if(currentGovernor == GOVERNOR_USERSPACE){
            domain->setFrequencyUserspace(currentFrequency);
            EXPECT_EQ(domain->getCurrentFrequencyUserspace(), currentFrequency);
        }
    }
}
