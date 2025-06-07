#ifndef STORE_STORE_HPP
#define STORE_STORE_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <mutex>

namespace store {

    class Store {
        public:
            Store();
            ~Store();

            bool add(const std::string& key, const std::string& value);
            bool remove(const std::string& key);
            bool update(const std::string& key, const std::string& value);
            std::optional<std::string> get(const std::string& key);
            std::vector<std::string> getAll();
        private:
            std::unordered_map<std::string, std::string> store;
            std::mutex mutex;
    };
}

#endif