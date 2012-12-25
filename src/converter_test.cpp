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

#define PATH_TEST_FILES "test_data/small"
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
  const string str("カレハ");
  vector<NgramConverter::Pair> results;

  lm.GetPairs(str, &results);

  bool found_kareha = false;
  bool found_kare = false;
  bool found_ka_1 = false;
  bool found_ka_2 = false;

  for (vector<NgramConverter::Pair>::const_iterator it = results.begin();
       it != results.end(); ++it) {
    if (it->src_str == "カレハ" &&
	it->dst_str == "カレハ/枯れ葉/名詞-普通名詞-一般/") {
      found_kareha = true;
    } else if (it->src_str == "カレ" && it->dst_str == "カレ/彼/代名詞/") {
      found_kare = true;
    } else if (it->src_str == "カ" && it->dst_str == "カ/か/助詞-係助詞/") {
      found_ka_1 = true;
    } else if (it->src_str == "カ" && it->dst_str ==
	       "カ/蚊/名詞-普通名詞-一般/") {
      found_ka_2 = true;
    }
  }

  EXPECT_TRUE(found_kareha);
  EXPECT_TRUE(found_kare);
  EXPECT_TRUE(found_ka_1);
  EXPECT_TRUE(found_ka_2);
}

TEST_F(LMTest, NgramTest) {
  vector<string> dst_strs;
  dst_strs.push_back("キョウ/今日/名詞-普通名詞-副詞可能/");
  dst_strs.push_back("ハ/は/助詞-係助詞/");

  vector<NgramConverter::NgramData> expected_data;
  NgramConverter::NgramData d1 = { 54493, 0, -3.951135, -0.470866 };
  NgramConverter::NgramData d2 = { 107004, 54493, -1.039273, -0.025837 };
  expected_data.push_back(d1);
  expected_data.push_back(d2);

  uint32_t context_id = 0;
  NgramConverter::NgramData ngram;
  for (size_t i = 0; i < dst_strs.size(); ++i) {
    uint32_t token_id;
    uint32_t new_context_id;

    EXPECT_EQ(expected_data[i].context_id, context_id);
    EXPECT_TRUE(lm.GetTokenId(dst_strs[i], &token_id));
    EXPECT_TRUE(lm.GetNgram(i+1, token_id, context_id, &new_context_id,
			    &ngram));
    EXPECT_TRUE(IsNear(ngram.score, expected_data[i].score));
    context_id = new_context_id;
  }
}

TEST_F(LMTest, ConvertTest) {
  string src = "シコル。";
  string dst;
  string expected = "今日はいい天気ですね。";
  NgramConverter::Converter converter(&lm);
  EXPECT_TRUE(converter.Convert(src, &dst));
  EXPECT_EQ(expected, dst);
}
