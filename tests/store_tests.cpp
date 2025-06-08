#include <gtest/gtest.h>
#include "store/store.hpp"
#include <thread>
#include <vector>
#include <iostream>

using namespace store;

class StoreTests : public ::testing::Test {
protected:
    Store::TimePoint current_time = std::chrono::system_clock::now();
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
    std::cout << "Running AddAndGet test" << std::endl;
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_FALSE(store.add("key1", "value2"));
    EXPECT_EQ(store.get("key1"), "value1");
    EXPECT_EQ(store.get("nonexistent"), std::nullopt);
}

TEST_F(StoreTests, Update) {
    std::cout << "Running Update test" << std::endl;
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_TRUE(store.update("key1", "value2"));
    EXPECT_EQ(store.get("key1"), "value2");
    EXPECT_FALSE(store.update("nonexistent", "value"));
}

TEST_F(StoreTests, Remove) {
    std::cout << "Running Remove test" << std::endl;
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_TRUE(store.remove("key1"));
    EXPECT_EQ(store.get("key1"), std::nullopt);
    EXPECT_FALSE(store.remove("nonexistent"));
}

TEST_F(StoreTests, GetAll) {
    std::cout << "Running GetAll test" << std::endl;
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_TRUE(store.add("key2", "value2"));
    auto keys = store.getAll();
    EXPECT_EQ(keys.size(), 2);
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key1") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key2") != keys.end());
}

TEST_F(StoreTests, Expiration) {
    std::cout << "Running Expiration test" << std::endl;
    EXPECT_TRUE(store.add("key1", "value1"));
    
    auto duration = std::chrono::milliseconds(10);
    EXPECT_TRUE(store.expire("key1", duration));
    
    auto ttl = store.getTTL("key1");
    EXPECT_TRUE(ttl.has_value());
    EXPECT_GE(ttl.value().count(), 0);
    EXPECT_LE(ttl.value().count(), 10);

    advance_time(std::chrono::milliseconds(20));
    store.cleanupExpired();

    EXPECT_EQ(store.get("key1"), std::nullopt);
}

TEST_F(StoreTests, Persist) {
    std::cout << "Running Persist test" << std::endl;
    EXPECT_TRUE(store.add("key1", "value1"));

    EXPECT_TRUE(store.expire("key1", std::chrono::milliseconds(10)));

    EXPECT_TRUE(store.persist("key1"));

    advance_time(std::chrono::milliseconds(20));

    EXPECT_EQ(store.get("key1"), "value1");
}

TEST_F(StoreTests, ExpireNonExistent) {
    std::cout << "Running ExpireNonExistent test" << std::endl;
    EXPECT_FALSE(store.expire("nonexistent", std::chrono::milliseconds(10)));
}

TEST_F(StoreTests, GetTTLNonExistent) {
    std::cout << "Running GetTTLNonExistent test" << std::endl;
    EXPECT_EQ(store.getTTL("nonexistent"), std::nullopt);
}

TEST_F(StoreTests, PersistNonExistent) {
    std::cout << "Running PersistNonExistent test" << std::endl;
    EXPECT_FALSE(store.persist("nonexistent"));
}

TEST_F(StoreTests, UpdateExpired) {
    std::cout << "Running UpdateExpired test" << std::endl;
    EXPECT_TRUE(store.add("key1", "value1"));

    EXPECT_TRUE(store.expire("key1", std::chrono::milliseconds(0)));

    advance_time(std::chrono::milliseconds(10));

    EXPECT_FALSE(store.update("key1", "value2"));
    EXPECT_EQ(store.get("key1"), std::nullopt);
}

TEST_F(StoreTests, GetAllWithExpired) {
    std::cout << "Running GetAllWithExpired test" << std::endl;
    EXPECT_TRUE(store.add("key1", "value1"));
    EXPECT_TRUE(store.add("key2", "value2"));

    EXPECT_TRUE(store.expire("key1", std::chrono::milliseconds(0)));

    advance_time(std::chrono::milliseconds(10));

    store.cleanupExpired();

    auto keys = store.getAll();
    EXPECT_EQ(keys.size(), 1);
    EXPECT_EQ(keys[0], "key2");
}

TEST_F(StoreTests, ThreadSafety) {
    std::cout << "Running ThreadSafety test" << std::endl;
    const int num_threads = 2;
    const int num_operations = 10;
    std::vector<std::thread> threads;

    std::cout << "Creating " << num_threads << " threads" << std::endl;

    for (int i = 0; i < num_threads; ++i) {
        std::cout << "Creating thread " << i << std::endl;
        threads.emplace_back([this, i, num_operations]() {
            std::cout << "Thread " << i << " started" << std::endl;
            for (int j = 0; j < num_operations; ++j) {
                std::string key = "key_" + std::to_string(i) + "_" + std::to_string(j);
                std::string value = "value_" + std::to_string(i) + "_" + std::to_string(j);
                
                std::cout << "Thread " << i << " operation " << j << std::endl;

                if (j % 2 == 0) {
                    store.add(key, value);
                } else {
                    store.get(key);
                }
            }
            std::cout << "Thread " << i << " finished" << std::endl;
        });
    }

    std::cout << "Waiting for threads to complete" << std::endl;

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << "All threads completed" << std::endl;

    auto keys = store.getAll();
    std::cout << "Found " << keys.size() << " keys in store" << std::endl;
    for (const auto& key : keys) {
        EXPECT_TRUE(store.get(key).has_value());
    }
}
