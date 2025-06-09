#include "server/metrics.hpp"
#include <sstream>
#include <iostream>

namespace server {
void Metrics::incrementCommand(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (command == "SET") {
        set_commands++;
    } else if (command == "GET") {
        get_commands++;
    } else if (command == "DEL") {
        del_commands++;
    } else if (command == "PERSIST") {
        persist_commands++;
    }
}

uint64_t Metrics::getCommandCount(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (command == "SET") {
        return set_commands;
    } else if (command == "GET") {
        return get_commands;
    } else if (command == "DEL") {
        return del_commands;
    } else if (command == "PERSIST") {
        return persist_commands;
    }
    return 0;
}

void Metrics::updateMemoryUsage(int64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "\n=== Metrics::updateMemoryUsage called ===" << std::endl;
    std::cout << "Current total memory: " << total_memory_bytes << " bytes" << std::endl;
    std::cout << "Requested change: " << bytes << " bytes" << std::endl;
    std::cout.flush();

    if (bytes < 0) {
        if (static_cast<uint64_t>(-bytes) > total_memory_bytes) {
            std::cout << "Memory underflow detected, resetting to 0" << std::endl;
            std::cout.flush();
            total_memory_bytes = 0;
        } else {
            total_memory_bytes -= static_cast<uint64_t>(-bytes);
            std::cout << "Removed memory usage: " << -bytes << " bytes" << std::endl;
            std::cout << "New total: " << total_memory_bytes << " bytes" << std::endl;
            std::cout.flush();
        }
    } else {
        total_memory_bytes += static_cast<uint64_t>(bytes);
        std::cout << "Added memory usage: " << bytes << " bytes" << std::endl;
        std::cout << "New total: " << total_memory_bytes << " bytes" << std::endl;
        std::cout.flush();
    }
    std::cout << "=== Metrics::updateMemoryUsage completed ===\n" << std::endl;
    std::cout.flush();
}

size_t Metrics::getMemoryUsage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "Metrics::getMemoryUsage called, returning " << total_memory_bytes << " bytes" << std::endl;
    std::cout.flush();
    return total_memory_bytes;
}

void Metrics::incrementConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    total_connections++;
    active_connections++;
}

void Metrics::decrementConnections() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_connections > 0) {
        active_connections--;
    }
}

uint64_t Metrics::getActiveConnections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_connections;
}

void Metrics::incrementAOFWrites() {
    std::lock_guard<std::mutex> lock(mutex_);
    aof_writes++;
}

void Metrics::incrementAOFErrors() {
    std::lock_guard<std::mutex> lock(mutex_);
    aof_errors++;
}

uint64_t Metrics::getAOFErrors() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return aof_errors;
}

uint64_t Metrics::getAOFWrites() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return aof_writes;
}

std::string Metrics::getPrometheusMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::stringstream ss;
    ss << "# HELP redis_commands_total Total number of commands processed\n";
    ss << "# TYPE redis_commands_total counter\n";
    ss << "redis_commands_total{command=\"set\"} " << set_commands << "\n";
    ss << "redis_commands_total{command=\"get\"} " << get_commands << "\n";
    ss << "redis_commands_total{command=\"del\"} " << del_commands << "\n";
    ss << "redis_commands_total{command=\"persist\"} " << persist_commands << "\n\n";
    
    ss << "# HELP redis_memory_bytes Total memory used in bytes\n";
    ss << "# TYPE redis_memory_bytes gauge\n";
    ss << "redis_memory_bytes " << total_memory_bytes << "\n\n";
    
    ss << "# HELP redis_connections_active Current number of active connections\n";
    ss << "# TYPE redis_connections_active gauge\n";
    ss << "redis_connections_active " << active_connections << "\n\n";
    
    ss << "# HELP redis_connections_total Total number of connections since server start\n";
    ss << "# TYPE redis_connections_total counter\n";
    ss << "redis_connections_total " << total_connections << "\n\n";
    
    ss << "# HELP redis_aof_writes_total Total number of AOF writes\n";
    ss << "# TYPE redis_aof_writes_total counter\n";
    ss << "redis_aof_writes_total " << aof_writes << "\n\n";
    
    ss << "# HELP redis_aof_errors_total Total number of AOF errors\n";
    ss << "# TYPE redis_aof_errors_total counter\n";
    ss << "redis_aof_errors_total " << aof_errors << "\n";
    
    return ss.str();
}
}
