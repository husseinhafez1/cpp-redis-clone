#include "server/server.hpp"
#include "server/aof_manager.hpp"
#include <iostream>

namespace server {
Server::Server(const std::string& host, unsigned short port)
    : host_(host)
    , port_(port)
    , running_(false)
    , io_context_()
    , acceptor_(io_context_) 
    , aof_manager_("redis.aof")
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
    aof_manager_.replay(
        [this](const std::string& key, const std::string& value) {
            store_.add(key, value);
        },
        [this](const std::string& key) {
            store_.remove(key);
        },
        [this](const std::string& key) {
            store_.persist(key);
        }
    );
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
            
            // Set a timeout for read operations
            socket.set_option(boost::asio::ip::tcp::socket::linger(true, 0));
            
            size_t bytes_read = socket.read_some(boost::asio::buffer(buffer.prepare(1024)), error);
            if (error) {
                if (error == boost::asio::error::eof) {
                    std::cout << "Client disconnected normally" << std::endl;
                } else {
                    std::cerr << "Error reading from socket: " << error.message() << std::endl;
                }
                break;
            }
            
            if (bytes_read == 0) {
                std::cout << "No data received, client might have disconnected" << std::endl;
                break;
            }
            
            buffer.commit(bytes_read);
            std::string chunk = boost::asio::buffer_cast<const char*>(buffer.data());
            complete_message += chunk;
            buffer.consume(buffer.size());
            
            // Add a maximum message size check
            if (complete_message.size() > 1024 * 1024) { // 1MB limit
                std::cerr << "Message too large, disconnecting client" << std::endl;
                break;
            }
            
            size_t pos = 0;
            auto value = resp::Parser::parse(complete_message, pos);
            if (!value) {
                if (complete_message.size() >= 2 && 
                    complete_message.substr(complete_message.size() - 2) == "\r\n") {
                    std::cout << "Failed to parse complete message" << std::endl;
                    complete_message.clear();
                }
                continue;
            }
            
            auto response = handleCommand(*value);
            std::string serialized = resp::Parser::serialize(response);
            
            boost::asio::write(socket, boost::asio::buffer(serialized), error);
            if (error) {
                std::cerr << "Error writing response: " << error.message() << std::endl;
                break;
            }
            
            complete_message.clear();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
    }
    
    try {
        socket.close();
    } catch (...) {
        // Ignore errors during socket closure
    }
    
    std::cout << "Client disconnected" << std::endl;
}

resp::Value Server::handleCommand(const resp::Value& command) {
    try {
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
                
                try {
                    if (store_.add(key, value)) {
                        aof_manager_.logSet(key, value);
                        return resp::SimpleString{"OK"};
                    } else {
                        return resp::Error{"ERR key already exists"};
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error in SET command: " << e.what() << std::endl;
                    return resp::Error{"ERR internal error"};
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
                
                try {
                    std::cout << "Attempting to delete key: " << key << std::endl;
                    if (store_.remove(key)) {
                        std::cout << "Successfully deleted key" << std::endl;
                        try {
                            if (aof_manager_.logDel(key)) {
                                std::cout << "Successfully logged DEL to AOF" << std::endl;
                            } else {
                                std::cerr << "Failed to log DEL to AOF" << std::endl;
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "Error logging DEL to AOF: " << e.what() << std::endl;
                        }
                        return resp::Integer{1};
                    } else {
                        std::cout << "Failed to delete key" << std::endl;
                        return resp::Integer{0};
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error in DEL command: " << e.what() << std::endl;
                    return resp::Error{"ERR internal error"};
                }
            }
            else if (cmd == "PERSIST") {
                if (array.size() != 2) {
                    return resp::Error{"ERR wrong number of arguments for PERSIST command"};
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
                
                try {
                    std::cout << "Attempting to persist key: " << key << std::endl;
                    if (store_.persist(key)) {
                        std::cout << "Successfully persisted key" << std::endl;
                        try {
                            if (aof_manager_.logPersist(key)) {
                                std::cout << "Successfully logged PERSIST to AOF" << std::endl;
                            } else {
                                std::cerr << "Failed to log PERSIST to AOF" << std::endl;
                            }
                        } catch (const std::exception& e) {
                            std::cerr << "Error logging PERSIST to AOF: " << e.what() << std::endl;
                        }
                        return resp::Integer{1};
                    } else {
                        std::cout << "Failed to persist key" << std::endl;
                        return resp::Integer{0};
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error in PERSIST command: " << e.what() << std::endl;
                    return resp::Error{"ERR internal error"};
                }
            }
            else {
                return resp::Error{"ERR unknown command"};
            }
        }
        return resp::Error{"ERR invalid command"};
    } catch (const std::exception& e) {
        std::cerr << "Error handling command: " << e.what() << std::endl;
        return resp::Error{"ERR internal error"};
    }
}

}