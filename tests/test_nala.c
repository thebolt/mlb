#include <nala.h>

#include "lite_broker/lite_broker.h"

TEST(trying_nala) {
  ASSERT_EQ(1, 1);

  one_function_mock(true);
  ASSERT_TRUE(one_function());
}

TEST(testcase2) { ASSERT_NE(1, 2); }