#include <utility>

#include "lm.h"
#include "word.h"

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

bool WordManager::Build(const string src, const LM& lm) {
  words_.resize(src.size() + 1);

  for (size_t pos = 0; pos < src.size(); pos = NextUtf8Pos(src, pos)) {
    vector<Word> results;

    if (!lm.GetWords(src.substr(pos), &results)) {
      return false;
    }
    for (vector<Word>::const_iterator it = results.begin();
	 it != results.end(); ++it) {
      words_[pos].push_back(*it);
    }
    if (results.size() == 0) {
      size_t next_pos = NextUtf8Pos(src, pos);
      Word unknown_word;
      uint32_t unk_token_id;
      if (!lm.GetTokenId(UNK_STR, &unk_token_id)) {
	return false;
      }
      unknown_word.src_str = src.substr(pos, next_pos - pos);
      unknown_word.dst_str = unknown_word.src_str;
      unknown_word.token_id = unk_token_id;
      
      words_[pos].push_back(unknown_word);
    }
  }

  Word eos_word;
  uint32_t eos_token_id;
  if (!lm.GetTokenId(EOS_STR, &eos_token_id)) {
    return false;
  }
  eos_word.token_id = eos_token_id;
  eos_word.src_str = " ";
  eos_word.dst_str = "";
  words_[src.size()].push_back(eos_word);
  return true;
}

void WordManager::GetWordsAt(int pos, const vector<Word>** words) const {
  *words = &words_[pos];
}

}  // namespace NgramConverter
