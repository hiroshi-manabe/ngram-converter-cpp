#ifndef NGRAM_CONVERTER_WORD_H_INCLUDED_
#define NGRAM_CONVERTER_WORD_H_INCLUDED_

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace NgramConverter {

class LM;

struct Word {
  string src_str;
  string dst_str;
  uint32_t token_id;
};

class WordManager {
 public:
  bool Build(const string src, const LM& lm);
  void GetWordsAt(int pos, const vector<Word>** words) const;

 private:
  vector<vector<Word> > words_;
};

}  // namespace NgramConverter

#endif  // NGRAM_CONVERTER_WORD_H_INCLUDED_
