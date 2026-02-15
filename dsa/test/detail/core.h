#pragma once

#include <cstdint>
#include <functional>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#include "singleton.h"

namespace testing {

class test_results : public testing::singleton<test_results> {
  friend class testing::singleton<test_results>;

private:
  int32_t total_tests_ = 0;
  int32_t passed_tests_ = 0;
  int32_t failed_tests_ = 0;
  int32_t passed_checks_ = 0;
  int32_t failed_checks_ = 0;
  mutable std::mutex mtx_;

  test_results() = default;
  ~test_results() { report(); }

public:
  void pass_check() {
    std::lock_guard<std::mutex> lock(mtx_);
    passed_checks_++;
  }

  void fail_check() {
    std::lock_guard<std::mutex> lock(mtx_);
    failed_checks_++;
  }

  void pass_test() {
    std::lock_guard<std::mutex> lock(mtx_);
    total_tests_++;
    passed_tests_++;
  }

  void fail_test() {
    std::lock_guard<std::mutex> lock(mtx_);
    total_tests_++;
    failed_tests_++;
  }

  void reset_checks() {
    std::lock_guard<std::mutex> lock(mtx_);
    passed_checks_ = 0;
    failed_checks_ = 0;
  }

  int32_t failed_checks() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return failed_checks_;
  }

  int32_t passed_checks() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return passed_checks_;
  }

  void report() const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::cout << "\n[============] " << total_tests_ << " test(s) ran.\n";
    std::cout << "[ ✓ PASSED   ] " << passed_tests_ << " test(s).\n";
    if (failed_tests_ > 0) {
      std::cout << "[ ✗ FAILED   ] " << failed_tests_ << " test(s).\n";
    }
    std::cout << "[============]\n";
  }
};

using functionT = std::function<bool()>;
struct test_case {
  std::string name{""};
  functionT func;
};

struct test_repository : public testing::singleton<test_repository> {
  friend class testing::singleton<test_repository>;

private:
  test_repository() = default;
  ~test_repository() = default;

  std::vector<test_case> tests_;
  mutable std::mutex mtx_;

public:
  void register_test(const std::string &name, functionT func) {
    std::lock_guard<std::mutex> lock(mtx_);
    tests_.push_back({name, func});
  }

  const std::vector<test_case> &get_tests() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return tests_;
  }
};

struct test_register {
  test_register(const std::string &name, functionT func) {
    test_repository::instance().register_test(name, func);
  }
};

template <class derived> struct test_base {
public:
  static bool run() {
    try {
      derived test;
      test.init();
      bool result = test.test_body();
      test.reset();
      return result;
    } catch (const std::exception &e) {
      std::cerr << "[ ✗ FAILED   ] exception in test_base: " << e.what()
                << "\n";
      return false;
    } catch (...) {
      std::cerr << "[ ✗ FAILED   ] unknown exception in test_base\n";
      return false;
    }
  }

protected:
  test_base() = default;
  ~test_base() = default;

  void init() {}
  void reset() {}

  bool test_body() { return true; }
};

static inline int run_all_tests() {
  auto &repo = test_repository::instance();
  auto &results = test_results::instance();

  auto &tests = repo.get_tests();
  std::cout << "=============================================\n";
  std::cout << "          Running All Registered Tests       \n";
  std::cout << "=============================================\n";

  for (const auto &test : tests) {
    std::cout << "[ RUN        ] " << test.name << "\n";
    results.reset_checks();
    bool test_passed = test.func();

    if (results.failed_checks() > 0) {
      test_passed = false;
    }

    if (test_passed) {
      results.pass_test();
      std::cout << "[   ✓   OK   ] " << test.name << "\n";
    } else {
      results.fail_test();
      std::cout << "[  ✗ FAILED  ] " << test.name << "\n";
    }
  }
  return results.failed_checks() > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

} // namespace testing