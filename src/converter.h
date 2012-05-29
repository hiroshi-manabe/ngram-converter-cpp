#ifndef NGRAM_CONVERTER_CONVERTER_H_INCLUDED_
#define NGRAM_CONVERTER_CONVERTER_H_INCLUDED_

#include <marisa.h>
#include <marisa/scoped-array.h>
#include <marisa/scoped-ptr.h>

#include <memory>
#include <stdint.h>
#include <string>

using std::string;
using marisa::scoped_ptr;
using marisa::scoped_array;

#define BLOCK_SIZE 128
#define MAX_INT_SCORE 255
#define BACKOFF_DISCOUNT 128
#define FLAGS_HAS_TOKEN_IDS 1
#define FLAGS_HAS_BACKOFF 2
#define MIN_SCORE -7.0
#define INVALID_SCORE -65536.0

#define EXT_TRIE ".trie"
#define EXT_INDEX ".index"
#define EXT_DATA ".data"

#define MAX_N 10

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
  bool LoadDics(string filename_prefix);
  bool LoadTrie(string filename);
  bool LoadNgrams(string filename_prefix);
  void GetUnigram(int token_id, NgramData* ngram);
  bool GetNgram(int n, int token_id, int context_id, int* new_context_id,
		NgramData* ngram);
 private:
  marisa::Trie trie_;
  scoped_array<uint8_t> ngram_data_;
  scoped_array<UnigramIndex> unigrams_;
  scoped_array<NgramIndex> ngram_indices_[MAX_N];
  size_t ngram_counts_[MAX_N];
  NgramData ngram_data_work_[BLOCK_SIZE];
  int n_;
};

}  // namespace NgramConverter
#endif  // NGRAM_CONVERTER_CONVERTER_H_INCLUDED_
