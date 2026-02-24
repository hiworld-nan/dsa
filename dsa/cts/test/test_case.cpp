#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>
#include <sstream>
#include <vector>

#include "compileTimeString.h"
#include "datastructure.h"
#include "test.h"

using namespace utils;
using namespace testing;

TEST(CompileTimeStringTest, CompileTimeStringAsTemplateParameter) {
    // 1. 测试 NamedObject：验证 CompileTimeString 作为模板参数传递，并且 name 成员正确
    {
        NamedObject<"Player1"> player1;
        NamedObject<"EnemyBoss"> enemy;

        // 验证 name 成员是 CompileTimeString 类型，并且值正确
        CHECK_COMPILE_TIME(player1.name == "Player1");
        CHECK_COMPILE_TIME(enemy.name == "EnemyBoss");

        // 验证不同的字符串产生不同的类型（通过类型特征）
        CHECK_COMPILE_TIME(!std::is_same_v<decltype(player1), decltype(enemy)>);
    }

    // 2. 测试 ConfigEntry：验证键值对模板参数
    {
        // 验证 key 成员正确
        CHECK_COMPILE_TIME(ConfigEntry<"max_connections", int32_t>::key == "max_connections");
        CHECK_COMPILE_TIME(ConfigEntry<"timeout", double>::key == "timeout");

        // 验证 value_type 正确
        CHECK_COMPILE_TIME(std::is_same_v<ConfigEntry<"max_connections", int32_t>::value_type, int32_t>);
        CHECK_COMPILE_TIME(std::is_same_v<ConfigEntry<"timeout", double>::value_type, double>);

        // 测试 getKeyAsCTS 方法返回正确的 CompileTimeString
        constexpr auto ctsKey = ConfigEntry<"max_connections", int32_t>::getKeyAsCTS();
        CHECK_COMPILE_TIME(ctsKey == "max_connections");
        CHECK_COMPILE_TIME(ctsKey.to_upper() == "MAX_CONNECTIONS");

        // 验证不同键的 ConfigEntry 是不同的类型
        CHECK_COMPILE_TIME(
            !std::is_same_v<ConfigEntry<"max_connections", int32_t>, ConfigEntry<"timeout", double>>);
    }

    // 3. 测试 CommandHandler：验证命令匹配逻辑
    {
        // 验证 command 成员正确
        CHECK_COMPILE_TIME(CommandHandler<"start">::command == "start");
        CHECK_COMPILE_TIME(CommandHandler<"stop">::command == "stop");

        // 测试 matches 方法
        CHECK_COMPILE_TIME(CommandHandler<"start">::matches("start"));
        CHECK_COMPILE_TIME(!CommandHandler<"start">::matches("stop"));
        CHECK_COMPILE_TIME(CommandHandler<"stop">::matches("stop"));
        CHECK_COMPILE_TIME(!CommandHandler<"stop">::matches("start"));

        // 验证不同的命令产生不同的类型
        CHECK_COMPILE_TIME(!std::is_same_v<CommandHandler<"start">, CommandHandler<"stop">>);
    }

    // 4. 测试 StringSwitch：验证变参模板和匹配逻辑
    {
        using MySwitch = StringSwitch<int32_t, "option1", "option2", "option3">;

        // 测试匹配结果
        auto result1 = MySwitch::match("option1");
        auto result2 = MySwitch::match("option2");
        auto result3 = MySwitch::match("option3");
        auto resultUnknown = MySwitch::match("unknown");

        // 验证匹配结果正确（根据 StringSwitch 的实现，匹配返回 T{static_cast<int32_t>(sizeof...(Rest) + 1)}）
        EXPECT_EQ(result1, 3);        // option1 匹配，应该是 3（sizeof...(Rest) + 1）
        EXPECT_EQ(result2, 2);        // option2 匹配，应该是 2
        EXPECT_EQ(result3, 1);        // option3 匹配，应该是 1
        EXPECT_EQ(resultUnknown, 0);  // 未知匹配，返回默认构造的 T{}

        // 测试只有一个 case 的特化
        using SingleSwitch = StringSwitch<bool, "only">;
        auto singleResult = SingleSwitch::match("only");
        auto singleUnknown = SingleSwitch::match("other");
        EXPECT_TRUE(singleResult);  // 根据实现，可能返回 true 或其他值，需要检查实际行为
        EXPECT_FALSE(singleUnknown);  // 默认构造的 bool 为 false
    }

    // 5. 测试 ObjectFactory：验证工厂模板参数
    {
        auto obj1 = ObjectFactory<"Player">::create();
        auto obj2 = ObjectFactory<"Enemy">::create(100, 50);

        // 验证返回类型（当前实现返回 std::string）
        CHECK_COMPILE_TIME(std::is_same_v<decltype(obj1), std::string>);
        CHECK_COMPILE_TIME(std::is_same_v<decltype(obj2), std::string>);

        // 验证字符串内容包含类型名称
        EXPECT_TRUE(obj1.find("Player") != std::string::npos);
        EXPECT_TRUE(obj2.find("Enemy") != std::string::npos);

        // 验证不同的类型名称产生不同的工厂类型
        CHECK_COMPILE_TIME(!std::is_same_v<ObjectFactory<"Player">, ObjectFactory<"Enemy">>);
    }

    // 6. 测试 CompileTimeString 作为模板参数时的编译时操作
    {
        // 使用 CompileTimeString 作为模板参数，并验证编译时操作
        constexpr CompileTimeString str1 = "Hello";
        constexpr CompileTimeString str2 = "World";

        // 编译时连接
        constexpr auto combined = str1 + " "_cs + str2.to_upper();
        CHECK_COMPILE_TIME(combined == "Hello WORLD");

        // 编译时哈希
        constexpr auto hashValue = combined.hash();
        CHECK_COMPILE_TIME(hashValue != 0);

        // 编译时查找
        CHECK_COMPILE_TIME(combined.contains("WORLD"));
        CHECK_COMPILE_TIME(!combined.contains("earth"));

        // 编译时替换
        constexpr auto replaced = combined.replace(' ', '_');
        CHECK_COMPILE_TIME(replaced == "Hello_WORLD");

        // 编译时切片
        constexpr auto firstWord = combined.substr(0, 5);
        CHECK_COMPILE_TIME(firstWord == "Hello");

        // 验证这些编译时操作的结果可以在模板参数中使用
        using TestType = NamedObject<replaced>;
        CHECK_COMPILE_TIME(TestType::name == "Hello_WORLD");
    }

    // 7. 边界情况测试
    {
        // 空字符串作为模板参数
        using EmptyNamedObject = NamedObject<"">;
        CHECK_COMPILE_TIME(EmptyNamedObject::name == "");
        CHECK_COMPILE_TIME(EmptyNamedObject::name.empty());
        CHECK_COMPILE_TIME(EmptyNamedObject::name.length() == 0);

        // 单字符字符串
        using SingleChar = NamedObject<"A">;
        CHECK_COMPILE_TIME(SingleChar::name == "A");
        CHECK_COMPILE_TIME(SingleChar::name.length() == 1);

        // 最大长度字符串（根据 CompileTimeString 的容量）
        constexpr CompileTimeString<10> maxStr("123456789");  // N-1 个字符
        using MaxNamedObject = NamedObject<"123456789">;
        CHECK_COMPILE_TIME(MaxNamedObject::name == "123456789");
        CHECK_COMPILE_TIME(MaxNamedObject::name.length() == 9);

        // 验证编译时字符串字面量作为模板参数
        using LiteralNamedObject = NamedObject<"test"_cs>;
        CHECK_COMPILE_TIME(LiteralNamedObject::name == "test");
    }

    // 8. 验证 CompileTimeString 作为模板参数的类型特征
    {
        // CompileTimeString 应该是字面类型
        CHECK_COMPILE_TIME(std::is_trivially_copyable_v<CompileTimeString<10>>);
        CHECK_COMPILE_TIME(std::is_standard_layout_v<CompileTimeString<10>>);
        CHECK_COMPILE_TIME(std::is_trivially_copyable_v<CompileTimeString<10>>);

        // 可以作为非类型模板参数传递
        CHECK_COMPILE_TIME([]<CompileTimeString S>() { return S == "test"; }.template operator()<"test">());

        // 不同的 CompileTimeString 参数产生不同的模板实例
        CHECK_COMPILE_TIME([]<CompileTimeString S>() { return true; }.template operator()<"a">());
        CHECK_COMPILE_TIME([]<CompileTimeString S>() { return true; }.template operator()<"b">());
    }

    return true;
}

TEST(CompileTimeStringTest, CtsFunction) {
    using std::operator""sv;

    // 1. 基本构造和赋值
    {
        constexpr auto str1 = CompileTimeString("Hello, World!");
        constexpr auto str2 = "Hello, World!"_cs;
        constexpr CompileTimeString<13> str21 = "test from sv"sv;
        constexpr CompileTimeString<19> str22{"test from sv again"sv};

        CHECK_COMPILE_TIME(str21 == "test from sv");
        CHECK_COMPILE_TIME(str22 == "test from sv again");
        CHECK_COMPILE_TIME(str1 == str2);

        // 从初始化列表构造
        constexpr CompileTimeString<4> str11{'1', '2', '3'};
        constexpr auto str12 = CompileTimeString<4>({'3', '2', '1'});
        constexpr CompileTimeString<4> str13 = {'3', '2', '1'};
        constexpr CompileTimeString<4> str14 = "123";
        constexpr CompileTimeString<4> str15{"123"};

        CHECK_COMPILE_TIME(str11 == "123");
        CHECK_COMPILE_TIME(str12 == "321");
        CHECK_COMPILE_TIME(str13 == "321");
        CHECK_COMPILE_TIME(str14 == "123");
        CHECK_COMPILE_TIME(str15 == "123");
    }

    // 2. 字符串分割
    {
        constexpr auto str = "Hello, World!"_cs;
        constexpr auto words = str.split_result(' ');
        CHECK_COMPILE_TIME(words.size() == 2);
        CHECK_COMPILE_TIME(words[0] == "Hello,");
        CHECK_COMPILE_TIME(words[1] == "World!");

        // 运行时 split 视图（需要运行时验证）
        auto split_view = str.split(' ');
        std::vector<std::string_view> parts;
        for (auto word : split_view) {
            parts.emplace_back(word.begin(), word.end());
        }
        EXPECT_EQ(parts.size(), 2);
        EXPECT_TRUE(parts[0] == "Hello,");
        EXPECT_TRUE(parts[1] == "World!");
    }

    // 3. 访问器和属性
    {
        constexpr auto str = "Hello, World!"_cs;
        CHECK_COMPILE_TIME(str.size() == 13);
        CHECK_COMPILE_TIME(str.length() == 13);
        CHECK_COMPILE_TIME(str.capacity() == 13);
        CHECK_COMPILE_TIME(!str.empty());
        CHECK_COMPILE_TIME(str[0] == 'H');
        CHECK_COMPILE_TIME(str.front() == 'H');
        CHECK_COMPILE_TIME(str.back() == '!');
        CHECK_COMPILE_TIME(str.at(7) == 'W');
        CHECK_COMPILE_TIME(str.data()[0] == 'H');

        // 转换操作符
        std::string_view sv = str;
        EXPECT_TRUE(sv.length() == 13);
        const char* cstr = str;
        EXPECT_TRUE(cstr[0] == 'H');
    }

    // 4. 查找操作
    {
        constexpr auto str = "Hello, World!"_cs;
        CHECK_COMPILE_TIME(str.find('W') == 7);
        CHECK_COMPILE_TIME(str.find("World") == 7);
        CHECK_COMPILE_TIME(str.find("Hello") == 0);
        CHECK_COMPILE_TIME(str.find("nonexistent") == str.npos);

        CHECK_COMPILE_TIME(str.contains('W'));
        CHECK_COMPILE_TIME(str.contains("World"));
        CHECK_COMPILE_TIME(!str.contains('z'));
        CHECK_COMPILE_TIME(!str.contains("goodbye"));

        CHECK_COMPILE_TIME(str.starts_with('H'));
        CHECK_COMPILE_TIME(str.starts_with("Hello"));
        CHECK_COMPILE_TIME(!str.starts_with('W'));

        CHECK_COMPILE_TIME(str.ends_with('!'));
        CHECK_COMPILE_TIME(str.ends_with("World!"));
        CHECK_COMPILE_TIME(!str.ends_with('H'));

        // find_first_of, find_last_of 等
        CHECK_COMPILE_TIME(str.find_first_of("aeiou") == 1);
        CHECK_COMPILE_TIME(str.find_last_of("aeiou") == 8);
    }

    // 5. 子串操作
    {
        constexpr auto str = "Hello, World!"_cs;
        constexpr auto substr1 = str.substr(0, 5);
        CHECK_COMPILE_TIME(substr1 == "Hello");

        constexpr auto substr2 = str.substr(7);
        CHECK_COMPILE_TIME(substr2 == "World!");

        constexpr auto substr3 = str.substr(7, 5);
        CHECK_COMPILE_TIME(substr3 == "World");

        // 编译时模板版本 substr
        constexpr auto substr4 = str.substr<0, 5>();
        CHECK_COMPILE_TIME(substr4 == "Hello");

        constexpr auto substr5 = str.substr<7>();
        CHECK_COMPILE_TIME(substr5 == "World!");
    }

    // 6. 字符串拼接
    {
        constexpr auto str3 = "Hello"_cs;
        constexpr auto str4 = "World"_cs;
        constexpr auto str5 = str3 + ", " + str4 + "!"_cs;
        CHECK_COMPILE_TIME(str5 == "Hello, World!");

        // 与字符串字面量拼接
        constexpr auto result2 = str3 + " World";
        CHECK_COMPILE_TIME(result2 == "Hello World");
    }

    // 7. 大小写转换
    {
        constexpr auto str = "Hello, World!"_cs;
        constexpr auto upper = str.to_upper();
        CHECK_COMPILE_TIME(upper == "HELLO, WORLD!");

        constexpr auto lower = upper.to_lower();
        CHECK_COMPILE_TIME(lower == "hello, world!");
    }

    // 8. 哈希计算
    {
        constexpr auto str1 = "Hello, World!"_cs;
        constexpr auto str2 = "Hello, World!"_cs;
        constexpr auto str3 = "Goodbye, World!"_cs;

        CHECK_COMPILE_TIME(str1.hash() == str2.hash());
        CHECK_COMPILE_TIME(str1.hash() != str3.hash());
    }

    // 9. 字符替换
    {
        constexpr auto str = "Hello, World!"_cs;
        constexpr auto replaced = str.replace('o', '0');
        CHECK_COMPILE_TIME(replaced == "Hell0, W0rld!");

        constexpr auto replaced2 = str.replace('l', 'L');
        CHECK_COMPILE_TIME(replaced2 == "HeLLo, WorLd!");
    }

    // 10. 复杂字符串操作
    {
        constexpr auto complex = ("Prefix "_cs + "Middle "_cs).to_upper() + "Suffix"_cs;
        CHECK_COMPILE_TIME(complex == "PREFIX MIDDLE Suffix");

        constexpr auto compiled_string = "The answer is: "_cs + "42"_cs.to_upper() + "!"_cs;
        CHECK_COMPILE_TIME(compiled_string == "The answer is: 42!");
    }

    // 11. 编译时检查（边界情况）
    {
        // 空字符串
        // constexpr auto empty = ""_cs;
        // CHECK_COMPILE_TIME(empty.empty());
        // CHECK_COMPILE_TIME(empty.length() == 0);
        // CHECK_COMPILE_TIME(empty.size() == 0);

        // 单字符字符串
        constexpr auto single = "a"_cs;
        CHECK_COMPILE_TIME(single.length() == 1);
        CHECK_COMPILE_TIME(single.front() == 'a' && single.back() == 'a');

        // 最大长度字符串（根据容量）
        constexpr auto maxlen = "123456789"_cs;  // 9 characters
        CHECK_COMPILE_TIME(maxlen.length() == 9);
    }

    // 12. 性能演示：所有操作都在编译时完成
    {
        // 下面的操作完全在编译时计算，运行时无开销
        constexpr auto result = ("Compile"_cs + "-time "_cs + "string"_cs).to_upper();
        CHECK_COMPILE_TIME(result == "COMPILE-TIME STRING");
    }

    return true;
}

// 测试编译期函数
TEST(CompileTimeStringTest, CtsBasicFunction) {
    // 测试 compileTimeStrlen
    CHECK_COMPILE_TIME(utils::compileTimeStrlen("") == 0);
    CHECK_COMPILE_TIME(utils::compileTimeStrlen("a") == 1);
    CHECK_COMPILE_TIME(utils::compileTimeStrlen("abc") == 3);

    // 测试 compileTimeIsspace
    CHECK_COMPILE_TIME(utils::compileTimeIsspace(' ') == true);
    CHECK_COMPILE_TIME(utils::compileTimeIsspace('\t') == true);
    CHECK_COMPILE_TIME(utils::compileTimeIsspace('\n') == true);
    CHECK_COMPILE_TIME(utils::compileTimeIsspace('\r') == true);
    CHECK_COMPILE_TIME(utils::compileTimeIsspace('\v') == true);
    CHECK_COMPILE_TIME(utils::compileTimeIsspace('\f') == true);
    CHECK_COMPILE_TIME(utils::compileTimeIsspace('a') == false);
    CHECK_COMPILE_TIME(utils::compileTimeIsspace('0') == false);
    CHECK_COMPILE_TIME(utils::compileTimeIsspace('\0') == false);

    return true;
}

// 测试构造函数和赋值
TEST(CompileTimeStringTest, BasicConstruction) {
    // 默认构造（应该不可用）
    // utils::CompileTimeString<10> empty; // 这应该无法编译

    // 从字符串字面量构造
    constexpr utils::CompileTimeString<6> str1("hello");
    CHECK_COMPILE_TIME(str1.length() == 5);
    CHECK_COMPILE_TIME(str1[0] == 'h');
    CHECK_COMPILE_TIME(str1[4] == 'o');

    // 从std::string_view构造
    constexpr std::string_view sv = "world";
    constexpr utils::CompileTimeString<6> str2(sv);
    CHECK_COMPILE_TIME(str2.length() == 5);
    CHECK_COMPILE_TIME(str2[0] == 'w');

    // 从初始化列表构造
    constexpr utils::CompileTimeString<5> str3{'t', 'e', 's', 't'};
    CHECK_COMPILE_TIME(str3.length() == 4);

    // 赋值操作符
    constexpr utils::CompileTimeString str4 = "assignment";
    CHECK_COMPILE_TIME(str4.length() == 10);

    // 从初始化列表赋值
    constexpr utils::CompileTimeString<3> str5 = {'n', 'e', 'w'};
    CHECK_COMPILE_TIME(str5.length() == 3);

    return true;
}

// 测试基本访问函数
TEST(CompileTimeStringTest, CTSBasicAccess) {
    constexpr utils::CompileTimeString<10> str("hello");

    // size 和 length
    CHECK_COMPILE_TIME(str.size() == 10);   // 容量
    CHECK_COMPILE_TIME(str.length() == 5);  // 实际长度
    CHECK_COMPILE_TIME(str.capacity() == 10);
    CHECK_COMPILE_TIME(!str.empty());

    // data
    CHECK_COMPILE_TIME(str.data()[0] == 'h');
    CHECK_COMPILE_TIME(str.data()[4] == 'o');
    CHECK_COMPILE_TIME(str.data()[5] == '\0');

    // 元素访问
    CHECK_COMPILE_TIME(str[0] == 'h');
    CHECK_COMPILE_TIME(str[4] == 'o');
    CHECK_COMPILE_TIME(str.front() == 'h');
    CHECK_COMPILE_TIME(str.back() == 'o');
    CHECK_COMPILE_TIME(str.at(1) == 'e');

    // 转换操作符
    std::string_view sv = str;
    EXPECT_TRUE(sv.length() == 5);
    const char* cstr = str;
    EXPECT_TRUE(cstr[0] == 'h');

    return true;
}

// 测试概念（编译期）
TEST(CompileTimeStringTest, CTSConcepts) {
    // 测试 AnyString
    CHECK_COMPILE_TIME(utils::AnyString<std::string>);
    CHECK_COMPILE_TIME(utils::AnyString<std::string_view>);
    CHECK_COMPILE_TIME(utils::AnyString<const char*>);
    CHECK_COMPILE_TIME(utils::AnyString<char[]>);
    CHECK_COMPILE_TIME(!utils::AnyString<int>);

    // 测试 StringViewLike
    CHECK_COMPILE_TIME(utils::StringViewLike<std::string_view>);
    CHECK_COMPILE_TIME(!utils::StringViewLike<std::string>);

    // 测试 CharArray
    CHECK_COMPILE_TIME(utils::CharArray<char[]>);
    CHECK_COMPILE_TIME(utils::CharArray<const char[]>);
    CHECK_COMPILE_TIME(!utils::CharArray<char*>);
    CHECK_COMPILE_TIME(!utils::CharArray<std::string>);

    // 测试 CharPointer
    CHECK_COMPILE_TIME(utils::CharPointer<char*>);
    CHECK_COMPILE_TIME(utils::CharPointer<const char*>);
    CHECK_COMPILE_TIME(!utils::CharPointer<int*>);
    CHECK_COMPILE_TIME(!utils::CharPointer<std::string>);

    // 测试 StringLike
    CHECK_COMPILE_TIME(utils::StringLike<std::string>);
    CHECK_COMPILE_TIME(utils::StringLike<std::string_view>);
    CHECK_COMPILE_TIME(utils::StringLike<utils::CompileTimeString<10>>);
    CHECK_COMPILE_TIME(!utils::StringLike<int>);

    return true;
}

// 测试迭代器
TEST(CompileTimeStringTest, CTSIterator) {
    constexpr utils::CompileTimeString<6> str("hello");

    // 前向迭代
    constexpr const char* expected = "hello";
    int i = 0;
    for (auto it = str.begin(); it != str.end(); ++it) {
        if (*it != expected[i++])
            return false;
    }

    // 反向迭代
    i = 4;
    for (auto it = str.rbegin(); it != str.rend(); ++it) {
        if (*it != expected[i--])
            return false;
    }

    // const 迭代器
    i = 0;
    for (auto it = str.cbegin(); it != str.cend(); ++it) {
        if (*it != expected[i++])
            return false;
    }

    return true;
}

// 测试比较操作符
TEST(CompileTimeStringTest, CTSCompare) {
    constexpr utils::CompileTimeString<6> str1("hello");
    constexpr utils::CompileTimeString<6> str2("hello");
    constexpr utils::CompileTimeString<6> str3("world");
    constexpr utils::CompileTimeString<7> str4("hello!");

    // 相等比较
    CHECK_COMPILE_TIME(str1 == str2);
    CHECK_COMPILE_TIME(str1 != str3);
    CHECK_COMPILE_TIME(str1 != str4);

    // 与字符串字面量比较
    CHECK_COMPILE_TIME(str1 == "hello");
    CHECK_COMPILE_TIME(str1 != "world");

    // 大小比较
    CHECK_COMPILE_TIME(str1 < str3);
    CHECK_COMPILE_TIME(str3 > str1);
    CHECK_COMPILE_TIME(str1 <= str2);
    CHECK_COMPILE_TIME(str1 >= str2);

    CHECK_COMPILE_TIME(str1 < "world");
    CHECK_COMPILE_TIME(str3 > "hello");

    return true;
}

// 测试字符串连接
TEST(CompileTimeStringTest, Concatenate) {
    constexpr utils::CompileTimeString<6> str1("hello");
    constexpr utils::CompileTimeString<6> str2("world");

    // CompileTimeString 连接
    constexpr auto result1 = str1 + str2;
    CHECK_COMPILE_TIME(result1.length() == 10);
    CHECK_COMPILE_TIME(result1 == "helloworld");

    // 与字符串字面量连接
    constexpr auto result2 = str1 + " world";
    CHECK_COMPILE_TIME(result2.length() == 11);
    CHECK_COMPILE_TIME(result2 == "hello world");

    return true;
}

// 测试子字符串操作
TEST(CompileTimeStringTest, SubString) {
    constexpr utils::CompileTimeString<12> str("hello world");

    // 运行时 substr
    constexpr auto sub1 = str.substr(6, 5);
    CHECK_COMPILE_TIME(sub1 == "world");

    constexpr auto sub2 = str.substr(6);  // 默认到结尾
    CHECK_COMPILE_TIME(sub2 == "world");

    constexpr auto sub3 = str.substr(0, 5);
    CHECK_COMPILE_TIME(sub3 == "hello");

    // 编译期 substr（模板版本）
    constexpr auto sub4 = str.substr<0, 5>();
    CHECK_COMPILE_TIME(sub4 == "hello");

    constexpr auto sub5 = str.substr<6>();
    CHECK_COMPILE_TIME(sub5 == "world");

    return true;
}

// 测试修剪操作
TEST(CompileTimeStringTest, Trim) {
    // 测试空白字符修剪
    constexpr utils::CompileTimeString<10> str1("  hello  ");

    constexpr auto trimmed1 = str1.trim();
    CHECK_COMPILE_TIME(trimmed1 == "hello");

    constexpr auto ltrimmed1 = str1.ltrim();
    CHECK_COMPILE_TIME(ltrimmed1 == "hello  ");

    constexpr auto rtrimmed1 = str1.rtrim();
    CHECK_COMPILE_TIME(rtrimmed1 == "  hello");

    // 测试指定字符修剪
    constexpr utils::CompileTimeString str2("xxxhelloxxx");

    constexpr auto trimmed2 = str2.trim('x');
    CHECK_COMPILE_TIME(trimmed2 == "hello");

    constexpr auto ltrimmed2 = str2.ltrim('x');
    CHECK_COMPILE_TIME(ltrimmed2 == "helloxxx");

    constexpr auto rtrimmed2 = str2.rtrim('x');
    CHECK_COMPILE_TIME(rtrimmed2 == "xxxhello");

    // 混合空白字符
    constexpr utils::CompileTimeString str3("\t\n hello \r\n");
    constexpr auto trimmed3 = str3.trim();
    CHECK_COMPILE_TIME(trimmed3 == "hello");

    return true;
}

// 测试查找操作
TEST(CompileTimeStringTest, Find) {
    constexpr utils::CompileTimeString str("hello world, hello c++");

    // find
    CHECK_COMPILE_TIME(str.find('o') == 4);
    CHECK_COMPILE_TIME(str.find('o', 5) == 7);
    CHECK_COMPILE_TIME(str.find("world") == 6);
    CHECK_COMPILE_TIME(str.find("hello", 1) == 13);
    CHECK_COMPILE_TIME(str.find("nonexistent") == str.npos);

    // rfind
    CHECK_COMPILE_TIME(str.rfind('o') == 17);
    CHECK_COMPILE_TIME(str.rfind("hello") == 13);

    // find_first_of
    CHECK_COMPILE_TIME(str.find_first_of("aeiou") == 1);  // 'e'
    CHECK_COMPILE_TIME(str.find_first_of(",+") == 11);    // ','

    // find_last_of
    CHECK_COMPILE_TIME(str.find_last_of("aeiou") == 17);  // 'o' in "c++"
    CHECK_COMPILE_TIME(str.find_last_of(",+") == 21);     // '+'

    // find_first_not_of
    CHECK_COMPILE_TIME(str.find_first_not_of("helo ") == 6);  // 'w'

    // find_last_not_of
    CHECK_COMPILE_TIME(str.find_last_not_of("+c") == 18);  // 'o' before "c++"

    return true;
}

// 测试切分操作
TEST(CompileTimeStringTest, Split) {
    constexpr utils::CompileTimeString<20> str("a,b,c,d,e");

    // split_result 使用单个分隔符
    constexpr auto result1 = str.split_result(',');
    CHECK_COMPILE_TIME(result1.size_ == 5);
    CHECK_COMPILE_TIME(result1[0] == "a");
    CHECK_COMPILE_TIME(result1[1] == "b");
    CHECK_COMPILE_TIME(result1[4] == "e");

    // split_result 使用多个分隔符
    constexpr utils::CompileTimeString<20> str2("a,b;c.d e");
    constexpr auto result2 = str2.split_result(",;. ");
    CHECK_COMPILE_TIME(result2.size_ == 5);
    CHECK_COMPILE_TIME(result2[0] == "a");
    CHECK_COMPILE_TIME(result2[4] == "e");

    // 测试空字段
    constexpr utils::CompileTimeString<20> str3("a,,c");
    constexpr auto result3 = str3.split_result(',');
    CHECK_COMPILE_TIME(result3.size_ == 3);
    CHECK_COMPILE_TIME(result3[0] == "a");
    CHECK_COMPILE_TIME(result3[1] == "");
    CHECK_COMPILE_TIME(result3[2] == "c");

    return true;
}

// 测试开始/结束检查
TEST(CompileTimeStringTest, EndWith) {
    constexpr utils::CompileTimeString<12> str("hello world");

    CHECK_COMPILE_TIME(str.starts_with('h'));
    CHECK_COMPILE_TIME(str.starts_with("hello"));
    CHECK_COMPILE_TIME(str.starts_with("hell"));
    CHECK_COMPILE_TIME(!str.starts_with('w'));
    CHECK_COMPILE_TIME(!str.starts_with("world"));

    CHECK_COMPILE_TIME(str.ends_with('d'));
    CHECK_COMPILE_TIME(str.ends_with("world"));
    CHECK_COMPILE_TIME(str.ends_with("orld"));
    CHECK_COMPILE_TIME(!str.ends_with('h'));
    CHECK_COMPILE_TIME(!str.ends_with("hello"));

    return true;
}

// 测试字符串修改操作
TEST(CompileTimeStringTest, Modify) {
    constexpr utils::CompileTimeString<12> str("hello world");

    // replace
    constexpr auto replaced1 = str.replace('l', 'L');
    CHECK_COMPILE_TIME(replaced1 == "heLLo worLd");

    // to_upper
    constexpr auto upper = str.to_upper();
    CHECK_COMPILE_TIME(upper == "HELLO WORLD");

    // to_lower
    constexpr utils::CompileTimeString<12> str2("HELLO WORLD");
    constexpr auto lower = str2.to_lower();
    CHECK_COMPILE_TIME(lower == "hello world");

    return true;
}

// 测试 contains 和 compare
TEST(CompileTimeStringTest, Contain) {
    constexpr utils::CompileTimeString<12> str("hello world");

    // contains
    CHECK_COMPILE_TIME(str.contains('o'));
    CHECK_COMPILE_TIME(str.contains("world"));
    CHECK_COMPILE_TIME(str.contains("lo wo"));
    CHECK_COMPILE_TIME(!str.contains('z'));
    CHECK_COMPILE_TIME(!str.contains("goodbye"));

    // compare
    CHECK_COMPILE_TIME(str.compare("hello world") == 0);
    CHECK_COMPILE_TIME(str.compare("hello") > 0);
    CHECK_COMPILE_TIME(str.compare("hello world!") < 0);

    constexpr utils::CompileTimeString<6> str2("hello");
    CHECK_COMPILE_TIME(str.compare(str2) > 0);

    return true;
}

// 测试 hash
TEST(CompileTimeStringTest, ConstexprHash) {
    constexpr utils::CompileTimeString<6> str1("hello");
    constexpr utils::CompileTimeString<6> str2("hello");
    constexpr utils::CompileTimeString<6> str3("world");

    CHECK_COMPILE_TIME(str1.hash() == str2.hash());
    CHECK_COMPILE_TIME(str1.hash() != str3.hash());

    // 测试空字符串的hash
    constexpr utils::CompileTimeString<10> empty("");
    CHECK_COMPILE_TIME(empty.hash() == 14695981039346656037ULL);

    return true;
}

// 测试 remove_prefix/suffix
TEST(CompileTimeStringTest, RemovePrefix) {
    constexpr utils::CompileTimeString<12> str("hello world");

    constexpr auto no_prefix = str.remove_prefix(6);
    CHECK_COMPILE_TIME(no_prefix == "world");

    constexpr auto no_suffix = str.remove_suffix(6);
    CHECK_COMPILE_TIME(no_suffix == "hello");

    return true;
}

// 测试用户定义字面量
TEST(CompileTimeStringTest, UserDefineLiteral) {
    using namespace utils;
    constexpr auto str = "test"_cs;
    CHECK_COMPILE_TIME(str.length() == 4);
    CHECK_COMPILE_TIME(str == "test");
    return true;
}

// 测试格式化（运行时测试）
TEST(CompileTimeStringTest, Format) {
    try {
        // 测试基本格式化
        std::string result1 = utils::Format<"Hello {}!">("world");
        EXPECT_TRUE(result1 == "Hello world!");

        // 测试多参数格式化
        std::string result2 = utils::Format<"{} + {} = {}">(2, 3, 5);
        EXPECT_TRUE(result2 == "2 + 3 = 5");

        // 测试格式说明符
        std::string result3 = utils::Format<"Value: {:.2f}">(3.14159);
        EXPECT_TRUE(result3 == "Value: 3.14");
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Format test failed: " << e.what() << "\n";
        return false;
    }
    return true;
}

// 运行时测试
TEST(CompileTimeStringTest, RuntimeTest) {
    // 测试 split views（这些需要运行时，因为 views 可能不是 constexpr）
    {
        utils::CompileTimeString<20> str("a,b,c");
        auto split_view = str.split(',');
        std::vector<std::string_view> parts;
        for (auto part : split_view) {
            parts.emplace_back(part.begin(), part.end());
        }

        EXPECT_TRUE(parts.size() == 3);
        EXPECT_TRUE(parts[0] == "a");
        EXPECT_TRUE(parts[1] == "b");
        EXPECT_TRUE(parts[2] == "c");
    }

    // 测试输出流操作符
    {
        utils::CompileTimeString<10> str("test");
        std::ostringstream oss;
        oss << str;
        EXPECT_TRUE(oss.str() == "test");
    }

    // 测试异常处理（赋值时字符串过长）
    {
        bool exception_thrown = false;
        try {
            utils::CompileTimeString<5> small;
            small = "this is too long";  // 应该抛出异常
        } catch (const std::length_error&) {
            exception_thrown = true;
        }
        EXPECT_TRUE(exception_thrown);
    }

    // 测试 to_string
    {
        utils::CompileTimeString<6> str("hello");
        std::string std_str = str.to_string();
        EXPECT_TRUE(std_str == "hello");
    }

    // 测试 swap
    {
        utils::CompileTimeString<10> str1("first");
        utils::CompileTimeString<10> str2("second");

        str1.swap(std::move(str2));
        EXPECT_TRUE(str1 == "second");
        // 注意：swap后str2的状态未定义，我们只检查str1
    }

    // 测试 clear
    {
        utils::CompileTimeString<10> str("hello");
        str.clear();
        EXPECT_TRUE(str.empty());
        EXPECT_TRUE(str.length() == 0);
    }
    return true;
}

TEST(CompileTimeStringTest, ExceptionTest) {
    // 测试 at 越界异常
    utils::CompileTimeString<10> str("hello");
    bool exception_thrown = false;
    try {
        (void)str.at(10);
    } catch (const std::out_of_range&) {
        exception_thrown = true;
    }
    EXPECT_TRUE(exception_thrown);

    // 测试赋值长度异常（已在 RuntimeTest 中测试，但这里再次测试）
    utils::CompileTimeString<5> small;
    exception_thrown = false;
    try {
        small = "too long";
    } catch (const std::length_error&) {
        exception_thrown = true;
    }
    EXPECT_TRUE(exception_thrown);
    return true;
}

TEST(CompileTimeStringTest, SplitStringDelimiter) {
    constexpr utils::CompileTimeString<20> str("hello::world::test");
    // split 字符串分隔符重载
    auto split_view = str.split("::");
    std::vector<std::string_view> parts;
    for (auto part : split_view) {
        parts.emplace_back(part.begin(), part.end());
    }
    EXPECT_EQ(parts.size(), 3);
    EXPECT_TRUE(parts[0] == "hello");
    EXPECT_TRUE(parts[1] == "world");
    EXPECT_TRUE(parts[2] == "test");
    return true;
}

TEST(CompileTimeStringTest, EdgeCases) {
    // 空字符串
    constexpr utils::CompileTimeString<10> empty("");
    CHECK_COMPILE_TIME(empty.empty());
    CHECK_COMPILE_TIME(empty.length() == 0);
    CHECK_COMPILE_TIME(empty.size() == 10);
    CHECK_COMPILE_TIME(empty.capacity() == 10);
    CHECK_COMPILE_TIME(empty.data()[0] == '\0');

    // 单字符
    constexpr utils::CompileTimeString<5> single("a");
    CHECK_COMPILE_TIME(single.length() == 1);
    CHECK_COMPILE_TIME(single[0] == 'a');
    CHECK_COMPILE_TIME(single.front() == 'a');
    CHECK_COMPILE_TIME(single.back() == 'a');

    // 最大长度字符串（N-1个字符）
    constexpr utils::CompileTimeString<5> maxlen("1234");
    CHECK_COMPILE_TIME(maxlen.length() == 4);
    CHECK_COMPILE_TIME(maxlen.size() == 5);
    CHECK_COMPILE_TIME(maxlen.capacity() == 5);

    // 编译时字符串字面量
    constexpr auto lit = "test"_cs;
    CHECK_COMPILE_TIME(lit.length() == 4);
    CHECK_COMPILE_TIME(lit == "test");
    return true;
}

TEST(CompileTimeStringTest, SplitResultMethods) {
    constexpr utils::CompileTimeString<10> str("a,b,c");
    constexpr auto result = str.split_result(',');
    CHECK_COMPILE_TIME(result.size() == 3);
    CHECK_COMPILE_TIME(result[0] == "a");
    CHECK_COMPILE_TIME(result[1] == "b");
    CHECK_COMPILE_TIME(result[2] == "c");
    // 测试迭代器
    int count = 0;
    for (const auto& part : result) {
        (void)part;
        ++count;
    }
    EXPECT_EQ(count, 3);
    return true;
}

int32_t main() { return testing::run_all_tests(); }