#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <functional>
#include <optional>

namespace server {

class AOFManager {
public:
    explicit AOFManager(const std::string& aof_file_path);
    
    ~AOFManager();

    AOFManager(const AOFManager&) = delete;
    AOFManager& operator=(const AOFManager&) = delete;

    bool logSet(const std::string& key, const std::string& value);
    bool logDel(const std::string& key);
    bool logPersist(const std::string& key);

    bool replay(std::function<void(const std::string&, const std::string&)> onSet,
                std::function<void(const std::string&)> onDel,
                std::function<void(const std::string&)> onPersist);

    bool isEnabled() const { return aof_file_.is_open(); }

private:
    std::string aof_file_path_;
    std::ofstream aof_file_;
    std::mutex mutex_;

    bool writeCommand(const std::string& command);
};

}