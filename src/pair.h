#ifndef NGRAM_CONVERTER_PAIR_H_INCLUDED_
#define NGRAM_CONVERTER_PAIR_H_INCLUDED_

#include <string>
#include <vector>

#include "converter.h"

using std::string;
using std::vector;

#define UNKNOWN_STRING "UNK"

namespace NgramConverter{

struct Pair {
  Pair(string src_str, string dst_str, int start_pos, int end_pos);
  string src_str;
  string dst_str;
  int start_pos;
  int end_pos;
};

class PairManager {
 public:
  bool Build(const string src, const LM& lm);
  vector<Pair> GetPairsAt(int pos);

 private:
  vector<vector<Pair> > pairs_;
};

}  // namespace NgramConverter

#endif  // NGRAM_CONVERTER_PAIR_H_INCLUDED_
