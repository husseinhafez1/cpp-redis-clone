#include "store/store.hpp"
#include "server/metrics.hpp"
#include <thread>
#include <chrono>
#include <iostream>

namespace store {

    size_t Store::calculateMemoryUsage(const std::string& key, const std::string& value) {
        std::cout << "\n=== Store::calculateMemoryUsage called ===" << std::endl;
        std::cout << "Key: '" << key << "'" << std::endl;
        std::cout << "Value: '" << value << "'" << std::endl;
        std::cout.flush();
        size_t key_size = key.size();
        size_t value_size = value.size();
        size_t expiry_size = sizeof(std::optional<std::chrono::system_clock::time_point>);
        size_t pair_size = sizeof(std::pair<std::string, std::pair<std::string, std::optional<std::chrono::system_clock::time_point>>>);
        size_t string_overhead = 2 * sizeof(std::string::size_type);
        
        size_t usage = key_size + value_size + expiry_size + pair_size + string_overhead;
        
        std::cout << "Memory calculation details:" << std::endl;
        std::cout << "  Key size: " << key_size << " bytes" << std::endl;
        std::cout << "  Value size: " << value_size << " bytes" << std::endl;
        std::cout << "  Expiry size: " << expiry_size << " bytes" << std::endl;
        std::cout << "  Pair size: " << pair_size << " bytes" << std::endl;
        std::cout << "  String overhead: " << string_overhead << " bytes" << std::endl;
        std::cout << "  Total usage: " << usage << " bytes" << std::endl;
        std::cout << "=== Store::calculateMemoryUsage completed ===\n" << std::endl;
        std::cout.flush();
        
        return usage;
    }

    void Store::startCleanupThread(std::chrono::seconds interval) {
        if (running_) return;
        running_ = true;
        cleanup_thread_ = std::thread(&Store::cleanupLoop, this, interval);
    }

    void Store::stopCleanupThread() {
        if (!running_) return;
        running_ = false;
        if (cleanup_thread_.joinable()) {
            cleanup_thread_.join();
        }
    }

    void Store::cleanupLoop(std::chrono::seconds interval) {
        while (running_) {
            std::this_thread::sleep_for(interval);
            if (!running_) break;
            cleanupExpired();
        }
    }

    bool Store::add(const std::string& key, const std::string& value) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        std::cout << "\n=== Store::add called ===" << std::endl;
        std::cout << "Key: '" << key << "'" << std::endl;
        std::cout << "Value: '" << value << "'" << std::endl;
        std::cout.flush();

        if (store.find(key) != store.end()) {
            std::cout << "Key already exists, returning false" << std::endl;
            std::cout.flush();
            return false;
        }

        std::cout << "Calculating memory usage..." << std::endl;
        std::cout.flush();
        size_t memory_usage = calculateMemoryUsage(key, value);
        
        std::cout << "Calling Metrics::updateMemoryUsage with " << memory_usage << " bytes..." << std::endl;
        std::cout.flush();
        server::Metrics::getInstance().updateMemoryUsage(memory_usage);
        
        std::cout << "Storing key-value pair..." << std::endl;
        std::cout.flush();
        store[key] = {value, std::nullopt};
        
        std::cout << "Key-value pair added successfully" << std::endl;
        std::cout << "=== Store::add completed ===\n" << std::endl;
        std::cout.flush();
        return true;
    }

    bool Store::remove(const std::string& key) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        std::cout << "Store::remove called for key='" << key << "'" << std::endl;
        if (store.find(key) == store.end()) {
            std::cout << "Key not found, returning false" << std::endl;
            return false;
        }
        size_t memory_usage = calculateMemoryUsage(key, store[key].value);
        std::cout << "Removing memory usage: " << memory_usage << " bytes" << std::endl;
        server::Metrics::getInstance().updateMemoryUsage(-memory_usage);
        store.erase(key);
        std::cout << "Key removed successfully" << std::endl;
        return true;
    }

    bool Store::update(const std::string& key, const std::string& value) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        std::cout << "Store::update called for key='" << key << "', value='" << value << "'" << std::endl;
        if (store.find(key) == store.end()) {
            std::cout << "Key not found, returning false" << std::endl;
            return false;
        }
        if (isExpired(key)) {
            std::cout << "Key is expired, removing it" << std::endl;
            remove(key);
            return false;
        }
        size_t old_memory_usage = calculateMemoryUsage(key, store[key].value);
        std::cout << "Removing old memory usage: " << old_memory_usage << " bytes" << std::endl;
        server::Metrics::getInstance().updateMemoryUsage(-old_memory_usage);
        size_t new_memory_usage = calculateMemoryUsage(key, value);
        std::cout << "Adding new memory usage: " << new_memory_usage << " bytes" << std::endl;
        server::Metrics::getInstance().updateMemoryUsage(new_memory_usage);
        store[key] = {value, std::nullopt};
        std::cout << "Key-value pair updated successfully" << std::endl;
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
        if (store[key].expiry) {
            size_t old_memory_usage = calculateMemoryUsage(key, store[key].value);
            std::cout << "Removing expiry memory usage: " << old_memory_usage << " bytes" << std::endl;
            server::Metrics::getInstance().updateMemoryUsage(-old_memory_usage);
            size_t new_memory_usage = calculateMemoryUsage(key, store[key].value);
            std::cout << "Adding new memory usage: " << new_memory_usage << " bytes" << std::endl;
            server::Metrics::getInstance().updateMemoryUsage(new_memory_usage);
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
                size_t memory_usage = calculateMemoryUsage(it->first, it->second.value);
                std::cout << "Removing expired key memory usage: " << memory_usage << " bytes" << std::endl;
                server::Metrics::getInstance().updateMemoryUsage(-memory_usage);
                it = store.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool Store::setExpiry(const std::string& key, std::chrono::seconds ttl) {
        std::lock_guard<std::recursive_mutex> lock(mutex);
        if (store.find(key) == store.end()) {
            return false;
        }
        if (store[key].expiry) {
            size_t old_memory_usage = calculateMemoryUsage(key, store[key].value);
            server::Metrics::getInstance().updateMemoryUsage(-old_memory_usage);
        }
        size_t new_memory_usage = calculateMemoryUsage(key, store[key].value);
        server::Metrics::getInstance().updateMemoryUsage(new_memory_usage);
        store[key].expiry = get_time_() + ttl;
        return true;
    }
}