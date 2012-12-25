#ifndef NGRAM_CONVERTER_PAIR_H_INCLUDED_
#define NGRAM_CONVERTER_PAIR_H_INCLUDED_

#include <map>
#include <string>
#include <vector>

#include "constants.h"

using std::map;
using std::string;
using std::vector;

#define UNKNOWN_STRING "UNK"

namespace NgramConverter {

class LM;

struct Pair {
  // Pair(string src_str, string dst_str, uint32_t token_id) :
  //  src_str(src_str), dst_str(dst_str), token_id(token_id) {};
  string src_str;
  string dst_str;
  uint32_t token_id;
  uint32_t length;
};

struct PairInflection {
  Pair pair;
  uint8_t inflection_code;
  PairInflection(uint32_t code, uint32_t prev_code, uint32_t length);
};

class PairManager {
 public:
  bool Build(const string src, const LM& lm);
  const map<uint32_t, vector<Pair> >& GetMapRef() const;

 private:
  map<uint32_t, vector<Pair> > pos_map_;
};

}  // namespace NgramConverter

#endif  // NGRAM_CONVERTER_PAIR_H_INCLUDED_
