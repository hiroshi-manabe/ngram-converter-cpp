#ifndef NGRAM_CONVERTER_LM_H_INCLUDED_
#define NGRAM_CONVERTER_LM_H_INCLUDED_

#include <marisa.h>
#include <marisa/scoped-array.h>
#include <marisa/scoped-ptr.h>

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

#include "word.h"

#define BLOCK_SIZE 128
#define WORD_SEPARATOR "/"
#define MAX_KEY_LEN 255
#define MAX_N 10
#define BOS_STR "<s>"
#define EOS_STR "</s>"
#define UNK_STR "<unk>"

#define MAX_INT_SCORE 255
#define BACKOFF_DISCOUNT 128
#define FLAGS_HAS_TOKEN_IDS 1
#define FLAGS_HAS_BACKOFF 2
#define MIN_SCORE -7.0
#define INVALID_SCORE -65536.0
#define INVALID_TOKEN_ID -1

#define EXT_KEY ".key"
#define EXT_KEY_WORD ".key_word"
#define EXT_WORD ".word"
#define EXT_INDEX ".index"
#define EXT_DATA ".data"
#define EXT_FILTER ".filter"

#define FILTER_COUNT 7
#define FILTER_BITS_PER_ELEM 10

namespace NgramConverter {

struct UnigramIndex {
  uint8_t score_int;
  uint8_t backoff_int;
};

struct NgramIndex {
  uint32_t token_id;
  uint32_t context_id;
  uint32_t data_offset;
};

struct NgramData {
  uint32_t token_id;
  uint32_t context_id;
  double score;
  double backoff;
};

class LM {
 public:
  bool LoadDicts(const std::string filename_prefix);
  bool LoadTries(const std::string filename_prefix);
  bool LoadNgrams(const std::string filename_prefix);
  bool LoadKeyWordData(const std::string filename_prefix);
  bool GetNgram(int n, uint32_t token_id, uint32_t context_id,
		uint32_t* new_context_id,
		NgramData* ngram_data) const;
  bool GetTokenId(const std::string dst, uint32_t* token_id) const;
  bool GetWords(const std::string src, std::vector<Word>* results) const;

 private:
  void GetUnigram(int token_id, NgramData* ngram) const;
  marisa::Trie trie_key_;
  marisa::Trie trie_key_word_;
  marisa::Trie trie_word_;
  marisa::scoped_array<uint8_t> ngram_data_;
  marisa::scoped_array<UnigramIndex> unigrams_;
  marisa::scoped_array<NgramIndex> ngram_indices_[MAX_N];
  marisa::scoped_array<uint8_t> filters_[MAX_N];
  std::vector<std::vector<std::size_t> > key_to_words_;
  
  size_t ngram_counts_[MAX_N];
  mutable NgramData ngram_data_work_[BLOCK_SIZE];
  int n_;
};

}  // namespace NgramConverter
#endif  // NGRAM_CONVERTER_LM_H_INCLUDED_
