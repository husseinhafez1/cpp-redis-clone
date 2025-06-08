#ifndef STORE_STORE_HPP
#define STORE_STORE_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <mutex>
#include <chrono>
#include <functional>

namespace store {

class Store {
    public:
        using TimePoint = std::chrono::system_clock::time_point;
        using TimeProvider = std::function<TimePoint()>;

        Store(TimeProvider time_provider = std::chrono::system_clock::now)
            : get_time_(std::move(time_provider)) {}
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

    private:
        std::recursive_mutex mutex;
        TimeProvider get_time_;
        struct KeyValue {
            std::string value;
            std::optional<TimePoint> expiry;
        };
        std::unordered_map<std::string, KeyValue> store;

        bool isExpired(const std::string& key);
};

}

#endif