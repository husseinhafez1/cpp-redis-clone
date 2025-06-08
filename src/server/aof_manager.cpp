#include "server/aof_manager.hpp"
#include "server/resp.hpp"
#include <iostream>

namespace server {
    AOFManager::AOFManager(const std::string& aof_file_path) : aof_file_path_(aof_file_path) {
        aof_file_.open(aof_file_path_, std::ios::out | std::ios::app | std::ios::binary);
        if (!aof_file_.is_open()) {
            std::cerr << "Failed to open AOF file: " << aof_file_path_ << std::endl;
            throw std::runtime_error("Failed to open AOF file");
        }
    }

    AOFManager::~AOFManager() {
        if (aof_file_.is_open()) {
            aof_file_.close();
        }
    }

    bool AOFManager::logSet(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            if (!aof_file_.is_open()) {
                std::cerr << "AOF file is not open" << std::endl;
                return false;
            }

            std::string command = "*3\r\n$3\r\nSET\r\n$" + 
                std::to_string(key.length()) + "\r\n" + key + "\r\n$" +
                std::to_string(value.length()) + "\r\n" + value + "\r\n";
            
            aof_file_ << command;
            if (!aof_file_.good()) {
                std::cerr << "Error writing to AOF file" << std::endl;
                return false;
            }
            
            aof_file_.flush();
            if (!aof_file_.good()) {
                std::cerr << "Error flushing AOF file" << std::endl;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error in logSet: " << e.what() << std::endl;
            return false;
        }
    }

    bool AOFManager::logDel(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            if (!aof_file_.is_open()) {
                std::cerr << "AOF file is not open" << std::endl;
                return false;
            }

            std::string command = "*2\r\n$3\r\nDEL\r\n$" + 
                std::to_string(key.length()) + "\r\n" + key + "\r\n";
            
            aof_file_ << command;
            if (!aof_file_.good()) {
                std::cerr << "Error writing to AOF file" << std::endl;
                return false;
            }
            
            aof_file_.flush();
            if (!aof_file_.good()) {
                std::cerr << "Error flushing AOF file" << std::endl;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error in logDel: " << e.what() << std::endl;
            return false;
        }
    }

    bool AOFManager::logPersist(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            if (!aof_file_.is_open()) {
                std::cerr << "AOF file is not open" << std::endl;
                return false;
            }

            std::string command = "*2\r\n$7\r\nPERSIST\r\n$" + 
                std::to_string(key.length()) + "\r\n" + key + "\r\n";
            
            aof_file_ << command;
            if (!aof_file_.good()) {
                std::cerr << "Error writing to AOF file" << std::endl;
                return false;
            }
            
            aof_file_.flush();
            if (!aof_file_.good()) {
                std::cerr << "Error flushing AOF file" << std::endl;
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error in logPersist: " << e.what() << std::endl;
            return false;
        }
    }

    bool AOFManager::replay(std::function<void(const std::string&, const std::string&)> onSet,
                            std::function<void(const std::string&)> onDel,
                            std::function<void(const std::string&)> onPersist) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!aof_file_.is_open()) {
            return false;
        }
        aof_file_.close();
        std::ifstream in_file(aof_file_path_, std::ios::binary);
        if (!in_file.is_open()) {
            return false;
        }

        std::string buffer;
        char chunk[1024];
        while (in_file.read(chunk, sizeof(chunk))) {
            buffer.append(chunk, in_file.gcount());
        }
        buffer.append(chunk, in_file.gcount());

        size_t pos = 0;
        while (pos < buffer.size()) {
            auto value = resp::Parser::parse(buffer, pos);
            if (!value) break;

            if (value->holds_alternative<resp::Array>()) {
                const auto& array = value->get<resp::Array>();
                if (array.empty()) continue;

                if (array[0].holds_alternative<resp::BulkString>()) {
                    const auto& cmd = array[0].get<resp::BulkString>();
                    if (!cmd) continue;

                    if (*cmd == "SET" && array.size() >= 3) {
                        const auto& key = array[1].get<resp::BulkString>();
                        const auto& value = array[2].get<resp::BulkString>();
                        if (key && value) {
                            onSet(*key, *value);
                        }
                    }
                    else if (*cmd == "DEL" && array.size() >= 2) {
                        const auto& key = array[1].get<resp::BulkString>();
                        if (key) {
                            onDel(*key);
                        }
                    }
                    else if (*cmd == "PERSIST" && array.size() >= 2) {
                        const auto& key = array[1].get<resp::BulkString>();
                        if (key) {
                            onPersist(*key);
                        }
                    }
                }
            }
        }

        in_file.close();
        aof_file_.open(aof_file_path_, std::ios::out | std::ios::app | std::ios::binary);
        return true;
    }

    bool AOFManager::writeCommand(const std::string& command) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!aof_file_.is_open()) {
            return false;
        }

        aof_file_ << command;
        aof_file_.flush();
        return true;
    }
}