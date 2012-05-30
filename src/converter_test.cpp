#include "converter.h"

#include <gtest/gtest.h>

#define PATH_TEST_FILES "../test_data/test"

class LMTest: public ::testing::Test
{
public:
  virtual void SetUp()
  {
    lm.LoadDics(PATH_TEST_FILES)
  }
  virtual void TearDown()
  {
  }
  NgramConverter::LM lm;
};

TEST_F(LMTest, DataTest) {
  lm.GetNgram();
}

