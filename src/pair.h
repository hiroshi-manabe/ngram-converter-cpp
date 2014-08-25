#ifndef NGRAM_CONVERTER_PAIR_H_INCLUDED_
#define NGRAM_CONVERTER_PAIR_H_INCLUDED_

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace NgramConverter {

class LM;

struct Pair {
  // Pair(string src_str, string dst_str, uint32_t token_id) :
  //  src_str(src_str), dst_str(dst_str), token_id(token_id) {};
  string src_str;
  string dst_str;
  uint32_t token_id;
};

class PairManager {
 public:
  bool Build(const string src, const LM& lm);
  void GetPairsAt(int pos, const vector<Pair>** pairs) const;

 private:
  vector<vector<Pair> > pairs_;
};

}  // namespace NgramConverter

#endif  // NGRAM_CONVERTER_PAIR_H_INCLUDED_
