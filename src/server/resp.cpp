#include "server/resp.hpp"
#include <stdexcept>

namespace server {
namespace resp {
    std::optional<Value> Parser::parse(const std::string& input) {
        if (input.empty()) return std::nullopt;
        
        size_t pos = 0;
        if (input[0] == '+') {
            return parseSimpleString(input, pos);
        } else if (input[0] == '-') {
            return parseError(input, pos);
        } else if (input[0] == ':') {
            return parseInteger(input, pos);
        } else if (input[0] == '$') {
            return parseBulkString(input, pos);
        } else if (input[0] == '*') {
            return parseArray(input, pos);
        }
        return std::nullopt;
    }

    std::optional<Value> Parser::parseSimpleString(const std::string& input, size_t& pos) {
        size_t end = input.find("\r\n", pos);
        if (end == std::string::npos) return std::nullopt;
        std::string result = input.substr(pos + 1, end - pos - 1);
        pos = end + 2;
        return Value(SimpleString(result));
    }

    std::optional<Value> Parser::parseError(const std::string& input, size_t& pos) {
        size_t end = input.find("\r\n", pos);
        if (end == std::string::npos) return std::nullopt;
        std::string result = input.substr(pos + 1, end - pos - 1);
        pos = end + 2;
        return Value(Error(result));
    }

    std::optional<Value> Parser::parseInteger(const std::string& input, size_t& pos) {
        size_t end = input.find("\r\n", pos);
        if (end == std::string::npos) return std::nullopt;
        std::string numStr = input.substr(pos + 1, end - pos - 1);
        pos = end + 2;
        return Value(Integer(std::stoll(numStr)));
    }

    std::optional<Value> Parser::parseBulkString(const std::string& input, size_t& pos) {
        size_t end = input.find("\r\n", pos);
        if (end == std::string::npos) return std::nullopt;
        
        int length = std::stoi(input.substr(pos + 1, end - pos - 1));
        pos = end + 2;
        
        if (length == -1) return Value(BulkString(std::nullopt));
        if (pos + length + 2 > input.size()) return std::nullopt;
        
        std::string result = input.substr(pos, length);
        pos += length + 2;
        return Value(BulkString(result));
    }

    std::optional<Value> Parser::parseArray(const std::string& input, size_t& pos) {
        size_t end = input.find("\r\n", pos);
        if (end == std::string::npos) return std::nullopt;
        
        int count = std::stoi(input.substr(pos + 1, end - pos - 1));
        pos = end + 2;
        
        Array result;
        for (int i = 0; i < count; i++) {
            if (pos >= input.size()) return std::nullopt;
            
            size_t elementPos = 0;
            auto element = parse(input.substr(pos));
            if (!element) return std::nullopt;
            
            result.push_back(*element);
            size_t elementSize = 0;
            if (element->holds_alternative<SimpleString>()) {
                elementSize = element->get<SimpleString>().value.size() + 3;
            } else if (element->holds_alternative<Error>()) {
                elementSize = element->get<Error>().value.size() + 3;
            } else if (element->holds_alternative<Integer>()) {
                elementSize = std::to_string(element->get<Integer>()).size() + 3;
            } else if (element->holds_alternative<BulkString>()) {
                const auto& bulk = element->get<BulkString>();
                if (!bulk) {
                    elementSize = 5; // "$-1\r\n"
                } else {
                    elementSize = bulk->size() + 5 + std::to_string(bulk->size()).size();
                }
            } else if (element->holds_alternative<Array>()) {
                const auto& array = element->get<Array>();
                elementSize = std::to_string(array.size()).size() + 3;
                for (const auto& subElement : array) {
                    elementSize += subElement.size() + 2;
                }
            }
            
            pos += elementSize;
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