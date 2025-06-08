#include <string>
#include <vector>
#include <variant>
#include <optional>

namespace server {
namespace resp {

struct SimpleString {
    std::string value;
    SimpleString(const std::string& s) : value(s) {}
    SimpleString(std::string&& s) : value(std::move(s)) {}
    size_t size() const { return value.size(); }
};

struct Error {
    std::string value;
    Error(const std::string& s) : value(s) {}
    Error(std::string&& s) : value(std::move(s)) {}
    size_t size() const { return value.size(); }
};

using Integer = int64_t;
using BulkString = std::optional<std::string>;

class Value;
using Array = std::vector<Value>;

class Value {
public:
    using VariantType = std::variant<SimpleString, Error, Integer, BulkString, Array>;
    
    template<typename T>
    Value(T&& value) : value_(std::forward<T>(value)) {}
    
    template<typename T>
    bool holds_alternative() const { return std::holds_alternative<T>(value_); }
    
    template<typename T>
    const T& get() const { return std::get<T>(value_); }
    
    size_t size() const {
        if (holds_alternative<SimpleString>()) {
            return get<SimpleString>().size();
        } else if (holds_alternative<Error>()) {
            return get<Error>().size();
        } else if (holds_alternative<BulkString>()) {
            const auto& bulk = get<BulkString>();
            return bulk ? bulk->size() : 0;
        } else if (holds_alternative<Array>()) {
            return get<Array>().size();
        }
        return 0;
    }
    
private:
    VariantType value_;
};

class Parser {
public:
    static std::optional<Value> parse(const std::string& input);
    static std::optional<Value> parse(const std::string& input, size_t& pos);
    static std::string serialize(const Value& value);

private:
    static std::optional<Value> parseSimpleString(const std::string& input, size_t& pos);
    static std::optional<Value> parseError(const std::string& input, size_t& pos);
    static std::optional<Value> parseInteger(const std::string& input, size_t& pos);
    static std::optional<Value> parseBulkString(const std::string& input, size_t& pos);
    static std::optional<Value> parseArray(const std::string& input, size_t& pos);
};

}
}