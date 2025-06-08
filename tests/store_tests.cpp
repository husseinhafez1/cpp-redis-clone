#include <gtest/gtest.h>
#include "store/store.hpp"
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>
#include <algorithm>

using namespace store;

class StoreTests : public ::testing::Test {
protected:
    std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();
    Store store{[this]() { return current_time; }};

    void advance_time(std::chrono::milliseconds duration) {
        current_time += duration;
    }

    void SetUp() override {
        std::cout << "Starting test: " << ::testing::UnitTest::GetInstance()->current_test_info()->name() << std::endl;
    }

    void TearDown() override {
        std::cout << "Finished test: " << ::testing::UnitTest::GetInstance()->current_test_info()->name() << std::endl;
    }
};

TEST_F(StoreTests, AddAndGet) {
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_FALSE(store.add("key1", "value2"));
    EXPECT_EQ(store.get("key1"), "value1");
    EXPECT_EQ(store.get("key2"), std::nullopt);
}

TEST_F(StoreTests, Remove) {
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_TRUE(store.remove("key1"));
    EXPECT_FALSE(store.remove("key1"));
    EXPECT_EQ(store.get("key1"), std::nullopt);
}

TEST_F(StoreTests, Update) {
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_TRUE(store.update("key1", "value2"));
    EXPECT_FALSE(store.update("key2", "value1"));
    EXPECT_EQ(store.get("key1"), "value2");
}

TEST_F(StoreTests, GetAll) {
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_TRUE(store.add("key2", "value2"));
    auto keys = store.getAll();
    EXPECT_EQ(keys.size(), 2);
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key1") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key2") != keys.end());
}

TEST_F(StoreTests, Expiry) {
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_TRUE(store.add("key2", "value2"));
    EXPECT_TRUE(store.add("key3", "value3"));
    
    // Set expiry for key1 and key2
    EXPECT_TRUE(store.setExpiry("key1", std::chrono::seconds(1)));
    EXPECT_TRUE(store.setExpiry("key2", std::chrono::seconds(2)));
    
    // Advance time by 1.5 seconds
    advance_time(std::chrono::milliseconds(1500));
    
    // key1 should be expired, key2 should still be valid
    EXPECT_EQ(store.get("key1"), std::nullopt);
    EXPECT_EQ(store.get("key2"), "value2");
    EXPECT_EQ(store.get("key3"), "value3");
}

TEST_F(StoreTests, Persist) {
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_TRUE(store.setExpiry("key1", std::chrono::seconds(1)));
    EXPECT_TRUE(store.persist("key1"));
    advance_time(std::chrono::milliseconds(1500));
    EXPECT_EQ(store.get("key1"), "value1");
}

TEST_F(StoreTests, GetTTL) {
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_TRUE(store.setExpiry("key1", std::chrono::seconds(10)));
    auto ttl = store.getTTL("key1");
    EXPECT_TRUE(ttl);
    EXPECT_GE(ttl->count(), 9);
    EXPECT_LE(ttl->count(), 10);
}

TEST_F(StoreTests, CleanupThread) {
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_TRUE(store.setExpiry("key1", std::chrono::seconds(1)));
    store.startCleanupThread(std::chrono::seconds(1));
    advance_time(std::chrono::milliseconds(1500));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(store.get("key1"), std::nullopt);
    store.stopCleanupThread();
}
