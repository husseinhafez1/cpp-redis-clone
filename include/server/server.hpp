#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/error.hpp>
#include <sstream>
#include <unordered_map>
#include "store/store.hpp"
#include "server/resp.hpp"

namespace server {
class Server {
public:
    Server(const std::string& host = "127.0.0.1", unsigned short port = 6379);
    ~Server();

    void start();
    void stop();

private:
    store::Store store_;
    std::string host_;
    unsigned short port_;

    std::atomic<bool> running_;
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;

    std::vector<std::thread> threads_;

    void accept_connections();
    void handle_client(boost::asio::ip::tcp::socket&& socket);

    std::vector<std::string> parseCommand(const std::string& input);
    resp::Value handleCommand(const resp::Value& command);
};
}