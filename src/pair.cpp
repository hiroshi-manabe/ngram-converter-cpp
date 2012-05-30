#include <utility>

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

  for (size_t pos = 0; pos < src.size(); pos = NextUtf8Pos(str, pos)) {
    vector<pair<string, string> > results;

    if (!lm.GetPairs(src, &results)) {
      return false;
    }
    for (vector<pair<string, string> >::const_iterator it = results.begin();
	 it != results.end(); ++it) {
      pairs_[pos].push_back(Pair(it->first, it->second,
				 pos, pos + it->first->size()));
    }
    if (results.size() == 0) {
      size_t next_pos = NextUtf8Pos(str, pos);
      pairs_[pos].push_back(Pair(UNKNOWN_STRING,
				 src.substr(pos, next_pos - pos),
				 pos, next_pos));
    }
  }
}

vector<Pair> GetPairsAt(int pos) {
  return pairs_[pos];
}

}  // namespace NgramConverter
