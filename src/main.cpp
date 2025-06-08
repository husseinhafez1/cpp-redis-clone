#include "server/server.hpp"
#include <iostream>

int main() {
    try {
        server::Server server("127.0.0.1", 6379);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}