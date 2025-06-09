#pragma once
#include <atomic>
#include <string>
#include <chrono>
#include <mutex>

namespace server {
class Metrics {
public:
    static Metrics& getInstance() {
        static Metrics instance;
        return instance;
    }
    void incrementCommand(const std::string& command);
    uint64_t getCommandCount(const std::string& command);
    void updateMemoryUsage(int64_t bytes);
    size_t getMemoryUsage() const;
    void incrementConnections();
    void decrementConnections();
    uint64_t getActiveConnections() const;
    void incrementAOFWrites();
    void incrementAOFErrors();
    uint64_t getAOFWrites() const;
    uint64_t getAOFErrors() const;
    std::string getPrometheusMetrics() const;

private:
    Metrics() = default;
    ~Metrics() = default;
    Metrics(const Metrics&) = delete;
    Metrics& operator=(const Metrics&) = delete;
    std::atomic<uint64_t> set_commands{0};
    std::atomic<uint64_t> get_commands{0};
    std::atomic<uint64_t> del_commands{0};
    std::atomic<uint64_t> persist_commands{0};
    std::atomic<uint64_t> expire_commands{0};
    std::atomic<uint64_t> ttl_commands{0};
    std::atomic<uint64_t> total_memory_bytes{0};
    std::atomic<uint64_t> active_connections{0};
    std::atomic<uint64_t> total_connections{0};
    std::atomic<uint64_t> aof_writes{0};
    std::atomic<uint64_t> aof_errors{0};
    mutable std::mutex mutex_;
};
}