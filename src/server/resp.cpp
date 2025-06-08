#include "server/resp.hpp"
#include <stdexcept>
#include <iostream>

namespace server {
namespace resp {
    static std::optional<Value> parse(const std::string& input, size_t& pos);

    std::optional<Value> Parser::parse(const std::string& input) {
        size_t pos = 0;
        return Parser::parse(input, pos);
    }

    std::optional<Value> Parser::parse(const std::string& input, size_t& pos) {
        if (pos >= input.size()) return std::nullopt;
        std::cout << "[RESP] parse: pos=" << pos << " input=[" << input.substr(pos) << "]\n";
        char type = input[pos];
        if (type == '+') {
            return Parser::parseSimpleString(input, pos);
        } else if (type == '-') {
            return Parser::parseError(input, pos);
        } else if (type == ':') {
            return Parser::parseInteger(input, pos);
        } else if (type == '$') {
            return Parser::parseBulkString(input, pos);
        } else if (type == '*') {
            return Parser::parseArray(input, pos);
        }
        return std::nullopt;
    }

    std::optional<Value> Parser::parseSimpleString(const std::string& input, size_t& pos) {
        size_t end = input.find("\r\n", pos);
        if (end == std::string::npos) return std::nullopt;
        std::string result = input.substr(pos + 1, end - pos - 1);
        pos = end + 2;
        std::cout << "[RESP] SimpleString: '" << result << "'\n";
        return Value(SimpleString(result));
    }

    std::optional<Value> Parser::parseError(const std::string& input, size_t& pos) {
        size_t end = input.find("\r\n", pos);
        if (end == std::string::npos) return std::nullopt;
        std::string result = input.substr(pos + 1, end - pos - 1);
        pos = end + 2;
        std::cout << "[RESP] Error: '" << result << "'\n";
        return Value(Error(result));
    }

    std::optional<Value> Parser::parseInteger(const std::string& input, size_t& pos) {
        size_t end = input.find("\r\n", pos);
        if (end == std::string::npos) return std::nullopt;
        std::string numStr = input.substr(pos + 1, end - pos - 1);
        pos = end + 2;
        std::cout << "[RESP] Integer: '" << numStr << "'\n";
        return Value(Integer(std::stoll(numStr)));
    }

    std::optional<Value> Parser::parseBulkString(const std::string& input, size_t& pos) {
        size_t end = input.find("\r\n", pos);
        if (end == std::string::npos) {
            std::cout << "[RESP] BulkString: waiting for length delimiter\n";
            return std::nullopt;
        }
        
        std::string length_str = input.substr(pos + 1, end - pos - 1);
        std::cout << "[RESP] BulkString: length_str='" << length_str << "'\n";
        int length = std::stoi(length_str);
        pos = end + 2;
        
        if (length == -1) {
            std::cout << "[RESP] BulkString: null\n";
            return Value(BulkString(std::nullopt));
        }
        
        if (pos + length + 2 > input.size()) {
            std::cout << "[RESP] BulkString: waiting for data (need " << length << " bytes)\n";
            return std::nullopt;
        }
        
        std::string result = input.substr(pos, length);
        pos += length;
        
        if (pos + 2 > input.size() || input.substr(pos, 2) != "\r\n") {
            std::cout << "[RESP] BulkString: waiting for final delimiter\n";
            return std::nullopt;
        }
        pos += 2;
        
        std::cout << "[RESP] BulkString: '" << result << "'\n";
        return Value(BulkString(result));
    }

    std::optional<Value> Parser::parseArray(const std::string& input, size_t& pos) {
        size_t end = input.find("\r\n", pos);
        if (end == std::string::npos) {
            std::cout << "[RESP] Array: waiting for length delimiter\n";
            return std::nullopt;
        }
        
        std::string length_str = input.substr(pos + 1, end - pos - 1);
        std::cout << "[RESP] Array: length_str='" << length_str << "'\n";
        int count = std::stoi(length_str);
        pos = end + 2;
        std::cout << "[RESP] Array: count=" << count << " pos=" << pos << "\n";
        
        Array result;
        for (int i = 0; i < count; i++) {   
            if (pos >= input.size()) {
                std::cout << "[RESP] Array: waiting for element " << i << " type\n";
                return std::nullopt;
            }
                    
            auto element = parse(input, pos);
            if (!element) {
                std::cout << "[RESP] Array: failed to parse element " << i << "\n";
                return std::nullopt;
            }
            result.push_back(*element);
        }
        
        return Value(result);
    }

    std::string Parser::serialize(const Value& value) {
        try {
            if (value.holds_alternative<SimpleString>()) {
                return "+" + value.get<SimpleString>().value + "\r\n";
            } else if (value.holds_alternative<Error>()) {
                return "-" + value.get<Error>().value + "\r\n";
            } else if (value.holds_alternative<Integer>()) {
                return ":" + std::to_string(value.get<Integer>()) + "\r\n";
            } else if (value.holds_alternative<BulkString>()) {
                const auto& bulk = value.get<BulkString>();
                if (!bulk) return "$-1\r\n";
                return "$" + std::to_string(bulk->length()) + "\r\n" + *bulk + "\r\n";
            } else if (value.holds_alternative<Array>()) {
                const auto& array = value.get<Array>();
                std::string result = "*" + std::to_string(array.size()) + "\r\n";
                for (const auto& element : array) {
                    result += serialize(element);
                }
                return result;
            }
            return "";
        } catch (const std::exception& e) {
            return "-ERR Internal error\r\n";
        }
    }
}
}