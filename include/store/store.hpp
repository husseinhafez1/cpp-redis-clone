#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <mutex>
#include <chrono>
#include <functional>
#include <thread>
#include <atomic>

namespace store {

class Store {
    public:
        using TimeProvider = std::function<std::chrono::system_clock::time_point()>;
        using Value = std::string;
        using Expiry = std::optional<std::chrono::system_clock::time_point>;

        Store(TimeProvider time_provider = std::chrono::system_clock::now)
            : time_provider_(time_provider), running_(false) {}
        ~Store() = default;

        bool add(const std::string& key, const std::string& value);

        bool remove(const std::string& key);

        bool update(const std::string& key, const std::string& value);

        std::optional<std::string> get(const std::string& key);
        std::vector<std::string> getAll();

        template<typename Rep, typename Period>
        bool expire(const std::string& key, std::chrono::duration<Rep, Period> ttl) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            if (store.find(key) == store.end()) {
                return false;
            }
            store[key].expiry = get_time_() + ttl;
            return true;
        }

        std::optional<std::chrono::seconds> getTTL(const std::string& key);

        bool persist(const std::string& key);

        void cleanupExpired();

        void startCleanupThread(std::chrono::seconds interval);
        void stopCleanupThread();

        bool setExpiry(const std::string& key, std::chrono::seconds ttl);

    private:
        struct Entry {
            Value value;
            Expiry expiry;
        };

        void cleanupLoop(std::chrono::seconds interval);
        std::chrono::system_clock::time_point get_time_() const { return time_provider_(); }

        std::unordered_map<std::string, Entry> store;
        std::recursive_mutex mutex;
        TimeProvider time_provider_;
        std::thread cleanup_thread_;
        std::atomic<bool> running_;

        bool isExpired(const std::string& key);
};

}