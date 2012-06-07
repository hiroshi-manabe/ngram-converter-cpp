#include "converter.h"
#include "lm.h"
#include "pair.h"

#include <cmath>
#include <cstdlib>
#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>

using std::pair;
using std::string;
using std::vector;

#define PATH_TEST_FILES "../test_data/test"
#define TEST_MAX_N 4

namespace {

bool IsNear(double score1, double score2) {
  return (fabs(score1 - score2) < fabs(MIN_SCORE / MAX_INT_SCORE));
}

}  // namespace

class LMTest: public ::testing::Test
{
public:
  virtual void SetUp()
  {
    lm.LoadDics(PATH_TEST_FILES);
  }
  virtual void TearDown()
  {
  }
  NgramConverter::LM lm;
};

TEST_F(LMTest, GetPairsTest) {
  const string str("かれは");
  vector<NgramConverter::Pair> results;

  lm.GetPairs(str, &results);

  bool found_kareha = false;
  bool found_kare = false;
  bool found_ka_1 = false;
  bool found_ka_2 = false;

  for (vector<NgramConverter::Pair>::const_iterator it = results.begin();
       it != results.end(); ++it) {
    if (it->src_str == "かれは" && it->dst_str == "枯葉") {
      found_kareha = true;
    } else if (it->src_str == "かれ" && it->dst_str == "彼") {
      found_kare = true;
    } else if (it->src_str == "か" && it->dst_str == "か") {
      found_ka_1 = true;
    } else if (it->src_str == "か" && it->dst_str == "蚊") {
      found_ka_2 = true;
    }
  }

  EXPECT_TRUE(found_kareha);
  EXPECT_TRUE(found_kare);
  EXPECT_TRUE(found_ka_1);
  EXPECT_TRUE(found_ka_2);
}

TEST_F(LMTest, NgramTest) {
  vector<string> src_strs;
  src_strs.push_back("きょう");
  src_strs.push_back("は");
  src_strs.push_back("い");
  src_strs.push_back("い");
  
  vector<string> dst_strs;
  dst_strs.push_back("今日");
  dst_strs.push_back("は");
  dst_strs.push_back("い");
  dst_strs.push_back("い");

  vector<NgramConverter::NgramData> expected_data;
  NgramConverter::NgramData d1 = { 29315, 0, -4.089, -0.287 };
  NgramConverter::NgramData d2 = { 1110, 29315, -0.840, -0.035 };
  NgramConverter::NgramData d3 = { 1073, 41356, -1.305, -0.191 };
  NgramConverter::NgramData d4 = { 1073, 20177, -0.320, INVALID_SCORE };
  expected_data.push_back(d1);
  expected_data.push_back(d2);
  expected_data.push_back(d3);
  expected_data.push_back(d4);

  uint32_t context_id = 0;
  NgramConverter::NgramData ngram;
  for (size_t i = 0; i < src_strs.size(); ++i) {
    uint32_t token_id;
    uint32_t new_context_id;

    EXPECT_EQ(expected_data[i].context_id, context_id);
    EXPECT_TRUE(lm.GetTokenId(src_strs[i], dst_strs[i], &token_id));
    EXPECT_TRUE(lm.GetNgram(i+1, token_id, context_id, &new_context_id,
			    &ngram));
    EXPECT_TRUE(IsNear(ngram.score, expected_data[i].score));
    context_id = new_context_id;
  }
}

TEST_F(LMTest, ConvertTest) {
  string src = "きょうはいいてんきですね。";
  string dst;
  string expected = "今日はいい天気ですね。";
  NgramConverter::Converter converter(&lm);
  EXPECT_TRUE(converter.Convert(src, &dst));
  EXPECT_EQ(expected, dst);
}
