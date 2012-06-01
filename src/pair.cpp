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

bool PairManager::Build(const string src, LM& lm) {
  pairs_.resize(src.size() + 1);

  for (size_t pos = 0; pos < src.size(); pos = NextUtf8Pos(src, pos)) {
    vector<Pair> results;

    if (!lm.GetPairs(src, &results)) {
      return false;
    }
    for (vector<Pair>::const_iterator it = results.begin();
	 it != results.end(); ++it) {
      pairs_[pos].push_back(PairWithPos(*it, pos, pos + it->src_str.size()));
    }
    if (results.size() == 0) {
      size_t next_pos = NextUtf8Pos(src, pos);
      pairs_[pos].push_back(PairWithPos(Pair(UNKNOWN_STRING,
					     src.substr(pos, next_pos - pos), 0),
					pos, next_pos));
    }
  }
  return true;
}

vector<PairWithPos> PairManager::GetPairsAt(int pos) {
  return pairs_[pos];
}

}  // namespace NgramConverter
