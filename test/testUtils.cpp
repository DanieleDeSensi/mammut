/**
 *  Different tests on utility functions.
 **/
#include <algorithm>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <mammut/mammut.hpp>
#include "gtest/gtest.h"

using namespace mammut::utils;

TEST(UtilitiesTest, Generic) {
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    EXPECT_TRUE(contains(v, 4));

    std::vector<int> z;
    z.push_back(2);
    z.push_back(4);
    EXPECT_TRUE(contains(z, v));

    // Insert to end
    insertFrontToEnd(z, v, 1);
    insertToEnd(z, v);

    EXPECT_EQ(v.back(), 4);
    v.pop_back();

    EXPECT_EQ(v.back(), 2);
    v.pop_back();

    EXPECT_EQ(v.back(), 2);
    v.pop_back();

    // Move to end
    moveFrontToEnd(z, v, 1);
    EXPECT_EQ(v.back(), 2);
    v.pop_back();
    EXPECT_TRUE(z.size() == 1);
    z.clear();

    z.push_back(2);
    z.push_back(4);
    moveToEnd(z, v);
    EXPECT_EQ(v.back(), 4);
    v.pop_back();

    EXPECT_EQ(v.back(), 2);
    v.pop_back();
    EXPECT_TRUE(z.empty());

    // Insert to front
    z.push_back(2);
    z.push_back(4);
    insertEndToFront(z, v, 1);
    insertToFront(z, v);

    EXPECT_EQ(v.front(), 2);
    v.erase(v.begin(), v.begin() + 1);

    EXPECT_EQ(v.front(), 4);
    v.erase(v.begin(), v.begin() + 1);

    EXPECT_EQ(v.front(), 4);
    v.erase(v.begin(), v.begin() + 1);

    // Move to front
    moveEndToFront(z, v, 1);
    EXPECT_EQ(v.front(), 4);
    v.erase(v.begin(), v.begin() + 1);
    EXPECT_TRUE(z.size() == 1);
    z.clear();

    z.push_back(2);
    z.push_back(4);
    moveToFront(z, v);
    EXPECT_EQ(v.front(), 2);
    v.erase(v.begin(), v.begin() + 1);

    EXPECT_EQ(v.front(), 4);
    v.erase(v.begin(), v.begin() + 1);
    EXPECT_TRUE(z.empty());
}
