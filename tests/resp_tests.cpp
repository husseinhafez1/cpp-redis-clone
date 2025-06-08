#include "server/resp.hpp"
#include <gtest/gtest.h>
#include <string>

namespace server {
namespace resp {
namespace test {

TEST(RespParserTest, ParseSimpleString) {
    auto result = Parser::parse("+OK\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds_alternative<SimpleString>());
    EXPECT_EQ(result->get<SimpleString>().value, "OK");

    result = Parser::parse("+\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds_alternative<SimpleString>());
    EXPECT_EQ(result->get<SimpleString>().value, "");
}

TEST(RespParserTest, ParseError) {
    auto result = Parser::parse("-Error message\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds_alternative<Error>());
    EXPECT_EQ(result->get<Error>().value, "Error message");
}

TEST(RespParserTest, ParseInteger) {
    auto result = Parser::parse(":42\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds_alternative<Integer>());
    EXPECT_EQ(result->get<Integer>(), 42);

    result = Parser::parse(":-123\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds_alternative<Integer>());
    EXPECT_EQ(result->get<Integer>(), -123);
}

TEST(RespParserTest, ParseBulkString) {
    auto result = Parser::parse("$6\r\nfoobar\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds_alternative<BulkString>());
    EXPECT_EQ(*result->get<BulkString>(), "foobar");

    result = Parser::parse("$-1\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds_alternative<BulkString>());
    EXPECT_FALSE(result->get<BulkString>().has_value());

    result = Parser::parse("$0\r\n\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds_alternative<BulkString>());
    EXPECT_EQ(*result->get<BulkString>(), "");
}

TEST(RespParserTest, ParseArray) {
    auto result = Parser::parse("*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds_alternative<Array>());
    const auto& array = result->get<Array>();
    EXPECT_EQ(array.size(), 2);
    ASSERT_TRUE(array[0].holds_alternative<BulkString>());
    ASSERT_TRUE(array[1].holds_alternative<BulkString>());
    EXPECT_EQ(*array[0].get<BulkString>(), "foo");
    EXPECT_EQ(*array[1].get<BulkString>(), "bar");

    result = Parser::parse("*0\r\n");
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->holds_alternative<Array>());
    EXPECT_EQ(result->get<Array>().size(), 0);
}

TEST(RespParserTest, Serialize) {
    Value simpleStr = SimpleString("OK");
    EXPECT_EQ(Parser::serialize(simpleStr), "+OK\r\n");

    Value error = Error("Error message");
    EXPECT_EQ(Parser::serialize(error), "-Error message\r\n");

    Value integer = Integer(123);
    EXPECT_EQ(Parser::serialize(integer), ":123\r\n");

    Value bulkStr = BulkString("foobar");
    EXPECT_EQ(Parser::serialize(bulkStr), "$6\r\nfoobar\r\n");

    Value nullBulk = BulkString(std::nullopt);
    EXPECT_EQ(Parser::serialize(nullBulk), "$-1\r\n");

    Array array = {
        SimpleString("foo"),
        Integer(123),
        BulkString("bar")
    };
    Value arrayValue = array;
    EXPECT_EQ(Parser::serialize(arrayValue), "*3\r\n+foo\r\n:123\r\n$3\r\nbar\r\n");
}

TEST(RespParserTest, InvalidInput) {
    EXPECT_FALSE(Parser::parse("").has_value());

    EXPECT_FALSE(Parser::parse("invalid\r\n").has_value());

    EXPECT_FALSE(Parser::parse("+OK").has_value());
    EXPECT_FALSE(Parser::parse("$6\r\nfoo").has_value());
    EXPECT_FALSE(Parser::parse("*2\r\n+foo").has_value());
}

}
}
}