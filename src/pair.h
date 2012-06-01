#ifndef NGRAM_CONVERTER_PAIR_H_INCLUDED_
#define NGRAM_CONVERTER_PAIR_H_INCLUDED_

#include <string>
#include <vector>

using std::string;
using std::vector;

#define UNKNOWN_STRING "UNK"

namespace NgramConverter {

class LM;

struct Pair {
  Pair(string src_str, string dst_str, uint32_t token_id) :
  src_str(src_str), dst_str(dst_str), token_id(token_id) {};
  string src_str;
  string dst_str;
  uint32_t token_id;
};

struct PairWithPos {
  PairWithPos(Pair pair, int start_pos, int end_pos) :
  pair(pair), start_pos(start_pos), end_pos(end_pos) {};
  Pair pair;
  int start_pos;
  int end_pos;
};

class PairManager {
 public:
  bool Build(const string src, LM& lm);
  vector<PairWithPos> GetPairsAt(int pos);

 private:
  vector<vector<PairWithPos> > pairs_;
};

}  // namespace NgramConverter

#endif  // NGRAM_CONVERTER_PAIR_H_INCLUDED_
