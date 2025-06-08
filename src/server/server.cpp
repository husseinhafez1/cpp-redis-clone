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
    store_.startCleanupThread(std::chrono::seconds(60));
    std::cout << "Server starting on " << host_ << ":" << port_ << std::endl;
    accept_connections();
    io_context_.run();
}

void Server::stop() {
    if (!running_) return;
    running_ = false;
    store_.stopCleanupThread();
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
        std::string complete_message;
        
        while (running_) {
            boost::asio::streambuf buffer;
            boost::system::error_code error;
            std::cout << "Waiting for data..." << std::endl;
            size_t bytes_read = socket.read_some(boost::asio::buffer(buffer.prepare(1024)), error);
            if (error) {
                std::cerr << "Error reading from socket: " << error.message() << std::endl;
                break;
            }
            std::cout << "Read " << bytes_read << " bytes" << std::endl;
            buffer.commit(bytes_read);
            
            std::string chunk = boost::asio::buffer_cast<const char*>(buffer.data());
            complete_message += chunk;
            buffer.consume(buffer.size());
            
            std::cout << "Current message: [" << complete_message << "]" << std::endl;
            
            std::cout << "Attempting to parse message..." << std::endl;
            size_t pos = 0;
            auto value = resp::Parser::parse(complete_message, pos);
            if (!value) {
                if (complete_message.size() >= 2 && 
                    complete_message.substr(complete_message.size() - 2) == "\r\n") {
                    std::cout << "Failed to parse complete message" << std::endl;
                    complete_message.clear();
                } else {
                    std::cout << "Message not complete yet, waiting for more data..." << std::endl;
                }
                continue;
            }
            
            std::cout << "Successfully parsed message" << std::endl;
            
            auto response = handleCommand(*value);
            std::string serialized = resp::Parser::serialize(response);
            std::cout << "Sending response: [" << serialized << "]" << std::endl;
            
            size_t written = boost::asio::write(socket, boost::asio::buffer(serialized), error);
            if (error) {
                std::cerr << "Error writing response: " << error.message() << std::endl;
                break;
            }
            std::cout << "Wrote " << written << " bytes" << std::endl;
            
            complete_message.clear();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
    }
    std::cout << "Client disconnected" << std::endl;
}

resp::Value Server::handleCommand(const resp::Value& command) {
    if (command.holds_alternative<resp::Array>()) {
        const auto& array = command.get<resp::Array>();
        if (array.empty()) {
            return resp::Error{"ERR empty command"};
        }
        
        std::string cmd;
        if (array[0].holds_alternative<resp::BulkString>()) {
            const auto& bulk = array[0].get<resp::BulkString>();
            if (!bulk) {
                return resp::Error{"ERR invalid command"};
            }
            cmd = *bulk;
        } else {
            return resp::Error{"ERR invalid command"};
        }
        
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
        
        if (cmd == "SET") {
            if (array.size() < 3) {
                return resp::Error{"ERR wrong number of arguments for SET command"};
            }
            
            std::string key;
            if (array[1].holds_alternative<resp::BulkString>()) {
                const auto& bulk = array[1].get<resp::BulkString>();
                if (!bulk) {
                    return resp::Error{"ERR invalid key"};
                }
                key = *bulk;
            } else {
                return resp::Error{"ERR invalid key"};
            }
            
            std::string value;
            if (array[2].holds_alternative<resp::BulkString>()) {
                const auto& bulk = array[2].get<resp::BulkString>();
                if (!bulk) {
                    return resp::Error{"ERR invalid value"};
                }
                value = *bulk;
            } else {
                return resp::Error{"ERR invalid value"};
            }
            
            if (store_.add(key, value)) {
                return resp::SimpleString{"OK"};
            } else {
                return resp::Error{"ERR key already exists"};
            }
        }
        else if (cmd == "GET") {
            if (array.size() != 2) {
                return resp::Error{"ERR wrong number of arguments for GET command"};
            }
            
            std::string key;
            if (array[1].holds_alternative<resp::BulkString>()) {
                const auto& bulk = array[1].get<resp::BulkString>();
                if (!bulk) {
                    return resp::Error{"ERR invalid key"};
                }
                key = *bulk;
            } else {
                return resp::Error{"ERR invalid key"};
            }
            
            auto value = store_.get(key);
            if (value) {
                return resp::BulkString{*value};
            } else {
                return resp::BulkString{std::nullopt};
            }
        }
        else if (cmd == "DEL") {
            if (array.size() != 2) {
                return resp::Error{"ERR wrong number of arguments for DEL command"};
            }
            
            std::string key;
            if (array[1].holds_alternative<resp::BulkString>()) {
                const auto& bulk = array[1].get<resp::BulkString>();
                if (!bulk) {
                    return resp::Error{"ERR invalid key"};
                }
                key = *bulk;
            } else {
                return resp::Error{"ERR invalid key"};
            }
            
            if (store_.remove(key)) {
                return resp::Integer{1};
            } else {
                return resp::Integer{0};
            }
        }
        else {
            return resp::Error{"ERR unknown command"};
        }
    }
    
    return resp::Error{"ERR invalid command"};
}

}