#include "store/store.hpp"

namespace store {

    Store::Store() = default;
    Store::~Store() = default;

    bool Store::add(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex);
        if (store.find(key) != store.end()) {
            return false;
        }
        store[key] = value;
        return true;
    }

    bool Store::remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex);
        if (store.find(key) == store.end()) {
            return false;
        }
        store.erase(key);
        return true;
    }

    bool Store::update(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex);
        if (store.find(key) == store.end()) {
            return false;
        }
        store[key] = value;
        return true;
    }

    std::optional<std::string> Store::get(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex);
        if (store.find(key) == store.end()) {
            return std::nullopt;
        }
        return store[key];
    }

    std::vector<std::string> Store::getAll() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::string> result;
        for (const auto& [key, value] : store) {
            result.push_back(key);
        }
        return result;
    }
}