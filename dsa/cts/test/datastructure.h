
#include <iostream>
#include <string>
#include <string_view>

#include "compileTimeString.h"

using namespace utils;
template <CompileTimeString Name>
struct NamedObject {
    static inline constexpr auto name = Name;

    void print() const { std::cout << "Object name: " << name << std::endl; }
};

template <CompileTimeString Key, class Value>
struct ConfigEntry {
    static inline constexpr auto key = Key;
    using value_type = Value;

    static void print() { std::cout << "Config: " << key << ", Type: " << typeid(Value).name() << std::endl; }

    static constexpr auto getKeyAsCTS() { return key; }
};

template <CompileTimeString Command>
struct CommandHandler {
    static inline constexpr auto command = Command;

    static void execute() { std::cout << "Executing command: " << command << std::endl; }

    static constexpr bool matches(std::string_view input) { return std::string_view(command) == input; }
};

template <class T, CompileTimeString... Cases>
struct StringSwitch;

template <class T>
struct StringSwitch<T> {
    static constexpr T match(std::string_view) {
        return T{};  // 默认情况
    }
};

template <class T, CompileTimeString Case, CompileTimeString... Rest>
struct StringSwitch<T, Case, Rest...> {
    static constexpr T match(std::string_view input) {
        if (std::string_view(Case) == input) {
            return T{static_cast<int32_t>(sizeof...(Rest) + 1)};
        }
        return StringSwitch<T, Rest...>::match(input);
    }
};

template <CompileTimeString TypeName>
struct ObjectFactory {
    template <class... Args>
    static auto create(Args&&... args) {
        // std::cout << "Creating object of type: " << TypeName << std::endl;
        return std::string("Object: ") + TypeName.data();
    }
};

template <class... Entries>
struct StringMap;

template <CompileTimeString Key, class Value>
struct StringMapEntry {
    static inline constexpr auto key = Key;
    using value_type = Value;
};

template <class... Entries>
struct StringMap {
    template <CompileTimeString Key>
    static constexpr auto find() {
        return nullptr;
    }

    static auto get(std::string_view key) {
        std::cout << "Looking up key: " << key << std::endl;
        return -1;
    }
};

struct Player1Type {
    static inline constexpr auto name = CompileTimeString("Player1");
};

struct EnemyBossType {
    static inline constexpr auto name = CompileTimeString("EnemyBoss");
};