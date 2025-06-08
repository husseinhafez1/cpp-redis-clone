#include "server/server.hpp"
#include <iostream>

namespace server {
Server::Server(const std::string& host, unsigned short port)
    : host_(host)
    , port_(port)
    , running_(false)
    , io_context_()
    , acceptor_(io_context_) 
{
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::address::from_string(host_),
        port_
    );
    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen();
}

Server::~Server() {
    stop();
}

void Server::start() {
    if (running_) return;
    running_ = true;
    std::cout << "Server starting on " << host_ << ":" << port_ << std::endl;
    accept_connections();
    io_context_.run();
}

void Server::stop() {
    if (!running_) return;
    running_ = false;
    io_context_.stop();
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    threads_.clear();
    std::cout << "Server stopped" << std::endl;
}

void Server::accept_connections() {
    if (!running_) return;
    acceptor_.async_accept(
        [this](const boost::system::error_code& error, boost::asio::ip::tcp::socket socket) {
            if (!error) {
                threads_.emplace_back(
                    [this, socket = std::move(socket)]() mutable {
                        handle_client(std::move(socket));
                    }
                );
            }
            accept_connections();
        }
    );
}

void Server::handle_client(boost::asio::ip::tcp::socket&& socket) {
    try {
        std::cout << "New client connected" << std::endl;
        while (running_) {
            boost::asio::streambuf buffer;
            boost::asio::read_until(socket, buffer, '\n');

            std::string message = boost::asio::buffer_cast<const char*>(buffer.data());
            message = message.substr(0, message.length() - 1);
            auto command = parseCommand(message);
            std::string response = handleCommand(command);
            boost::asio::write(socket, boost::asio::buffer(response + "\n"));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
    }
    std::cout << "Client disconnected" << std::endl;
}

std::vector<std::string> Server::parseCommand(const std::string& input) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(input);
    while (std::getline(tokenStream, token, ' ')) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

std::string Server::handleCommand(const std::vector<std::string>& command) {
    if (command.empty()) return "ERROR: Empty command";
    std::string cmd = command[0];
    if (cmd == "SET" && command.size() == 3) {
        store_.add(command[1], command[2]);
        return "OK";
    } else if (cmd == "GET" && command.size() == 2) {
        auto value = store_.get(command[1]);
        if (!value) return "(nil)";
        return *value;
    } else if (cmd == "DEL" && command.size() == 2) {
        bool deleted = store_.remove(command[1]);
        return deleted ? "1" : "0";
    } else if (cmd == "EXISTS" && command.size() == 2) {
        auto value = store_.get(command[1]);
        return value ? "1" : "0";
    }
    return "ERROR: Unknown command or wrong number of arguments";
}

}