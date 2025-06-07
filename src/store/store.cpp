#include "store/store.hpp"

namespace store {

    bool Store::add(const std::string& key, const std::string& value) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (store.find(key) != store.end()) {
            return false;
        }
        store[key] = {value, std::nullopt};
        return true;
    }

    bool Store::remove(const std::string& key) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (store.find(key) == store.end()) {
            return false;
        }
        store.erase(key);
        return true;
    }

    bool Store::update(const std::string& key, const std::string& value) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (store.find(key) == store.end()) {
            return false;
        }
        if (isExpired(key)) {
            remove(key);
            return false;
        }
        store[key] = {value, std::nullopt};
        return true;
    }

    std::optional<std::string> Store::get(const std::string& key) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (store.find(key) == store.end()) {
            return std::nullopt;
        }
        if (isExpired(key)) {
            remove(key);
            return std::nullopt;
        }
        return store[key].value;
    }

    std::vector<std::string> Store::getAll() {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        std::vector<std::string> result;
        for (const auto& [key, _] : store) {
            if (isExpired(key)) {
                remove(key);
                continue;
            }
            result.push_back(key);
        }
        return result;
    }

    bool Store::expire_at(const std::string& key, TimePoint expiry) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (store.find(key) == store.end()) {
            return false;
        }
        store[key].expiry = expiry;
        return true;
    }

    std::optional<std::chrono::seconds> Store::getTTL(const std::string& key) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (store.find(key) == store.end()) {
            return std::nullopt;
        }
        if (store[key].expiry) {
            return std::chrono::duration_cast<std::chrono::seconds>(store[key].expiry.value() - get_time_());
        }
        return std::nullopt;
    }

    bool Store::persist(const std::string& key) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (store.find(key) == store.end()) {
            return false;
        }
        store[key].expiry = std::nullopt;
        return true;
    }

    bool Store::isExpired(const std::string& key) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (store.find(key) == store.end()) {
            return false;
        }
        return store[key].expiry && store[key].expiry.value() < get_time_();
    }

    void Store::cleanupExpired() {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        for (auto it = store.begin(); it != store.end();) {
            if (isExpired(it->first)) {
                it = store.erase(it);
            } else {
                ++it;
            }
        }
    }
}