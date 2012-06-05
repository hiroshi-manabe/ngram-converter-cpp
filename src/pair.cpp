#include <utility>

#include "lm.h"
#include "pair.h"

using std::pair;

namespace {

int NextUtf8Pos(const string str, int pos) {
  ++pos;
  while ((str[pos] & 0xc0) == 0x80) {
    ++pos;
  }
  return pos;
}

}  // namespace

namespace NgramConverter{

bool PairManager::Build(const string src, const LM& lm) {
  pairs_.resize(src.size() + 1);

  for (size_t pos = 0; pos < src.size(); pos = NextUtf8Pos(src, pos)) {
    vector<Pair> results;

    if (!lm.GetPairs(src, &results)) {
      return false;
    }
    for (vector<Pair>::const_iterator it = results.begin();
	 it != results.end(); ++it) {
      pairs_[pos].push_back(*it);
    }
    if (results.size() == 0) {
      size_t next_pos = NextUtf8Pos(src, pos);
      Pair unknown_pair;
      if (lm.GetSpecialPair(UNK_STR, &unknown_pair)) {
	return false;
      }
      unknown_pair.src_str = src.substr(pos, next_pos - pos);
      unknown_pair.dst_str = unknown_pair.dst_str;
      
      pairs_[pos].push_back(unknown_pair);
    }
  }

  Pair eos_pair;
  if (lm.GetSpecialPair(EOS_STR, &eos_pair)) {
    return false;
  }
  pairs_[src.size()].push_back(eos_pair);
  return true;
}

void PairManager::GetPairsAt(int pos, const vector<Pair>** pairs) const {
  *pairs = &pairs_[pos];
}

}  // namespace NgramConverter
