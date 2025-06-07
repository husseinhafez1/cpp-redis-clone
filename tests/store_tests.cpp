#include <gtest/gtest.h>
#include "store/store.hpp"

class StoreTests : public ::testing::Test {
protected:
    store::Store store;
    void SetUp() override {

    }
    void TearDown() override {

    }
};

TEST_F(StoreTests, AddKeyValue) {
    EXPECT_TRUE(store.add("key", "value"));
    EXPECT_FALSE(store.add("key", "value"));
}

TEST_F(StoreTests, GetKeyValue) {
    store.add("key", "value");
    auto result = store.get("key");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "value");

    auto nonexistent = store.get("nonexistent");
    EXPECT_FALSE(nonexistent.has_value());
}

TEST_F(StoreTests, UpdateKeyValue) {
    store.add("key", "value");
    EXPECT_TRUE(store.update("key", "new_value"));
    auto result = store.get("key");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "new_value");
    EXPECT_FALSE(store.update("nonexistent", "new_value"));
}

TEST_F(StoreTests, RemoveKeyValue) {
    store.add("key", "value");
    EXPECT_TRUE(store.remove("key"));
    auto result = store.get("key");
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(store.remove("nonexistent"));
}

TEST_F(StoreTests, GetAllKeys) {
    auto empty_result = store.getAll();
    EXPECT_TRUE(empty_result.empty());
    store.add("key1", "value1");
    store.add("key2", "value2");
    store.add("key3", "value3");
    auto keys = store.getAll();
    EXPECT_EQ(keys.size(), 3);
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key1") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key2") != keys.end());
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key3") != keys.end());
}
