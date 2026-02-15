#include "test.h"

#include <type_traits>
#include <vector>

TEST(MathTest, Addition) {
  EXPECT_EQ(1 + 1, 2);
  EXPECT_GT(3, 2);
  EXPECT_LT(1, 2);
  return true;
}

TEST(MathTest, Multiplication) {
  EXPECT_EQ(2 * 3, 6);
  EXPECT_EQ(2 * 2, 4);
  return true;
}

TEST(CompileTest, BasicChecks) {
  CHECK_COMPILE_TIME(sizeof(int) == 4);
  CHECK_COMPILE_TIME(std::is_integral_v<int>);
  return true;
}

class VectorTest : public testing::test_base<VectorTest> {
public:
  std::vector<int> vec;

  void init() {
    vec = {1, 2, 3};
    std::cout << "[    init    ] Vector initialized\n";
  }

  void reset() {
    vec.clear();
    std::cout << "[    reset   ] Vector cleared\n";
  }
};

TEST_F(VectorTest, SizeCheck) {
  EXPECT_EQ(vec.size(), static_cast<size_t>(3));
  EXPECT_FALSE(vec.empty());
  return true;
}

TEST_F(VectorTest, ElementCheck) {
  EXPECT_EQ(vec[0], 1);
  EXPECT_EQ(vec[1], 2);
  EXPECT_EQ(vec[2], 3);
  return true;
}

TEST(CompileTest, BasicTest) {
  int32_t a = 10, b = 0;
  CHECK_EQ(a, b);
  CHECK_LE(2, 1);
  CHECK_COMPILE_TIME(10 == 10);
  CHECK(a == b);
}

int32_t main() { return testing::run_all_tests(); }