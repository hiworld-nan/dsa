#pragma once

namespace testing {

template <class T>
struct singleton {
public:
    static T &instance() {
        static T ins;
        return ins;
    }

protected:
    singleton() = default;
    ~singleton() = default;

private:
    singleton(singleton &&) = delete;
    singleton(const singleton &) = delete;

    singleton &operator=(singleton &&) = delete;
    singleton &operator=(const singleton &) = delete;
};

}  // namespace testing