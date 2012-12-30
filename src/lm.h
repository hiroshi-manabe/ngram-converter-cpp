#ifndef NGRAM_CONVERTER_LM_H_INCLUDED_
#define NGRAM_CONVERTER_LM_H_INCLUDED_

#include <marisa.h>
#include <marisa/scoped-array.h>
#include <marisa/scoped-ptr.h>

#include <memory>
#include <stdint.h>
#include <string>

#include "constants.h"
#include "pair.h"

using std::pair;
using std::string;
using std::vector;

using marisa::scoped_ptr;
using marisa::scoped_array;

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

struct VerbData {
  uint32_t main_verb;
  uint32_t potential_verb;
  uint32_t inflection_tail;
};

struct TekeiData {
  uint32_t tekei_id;
  uint32_t normalized_id;
};

class LM {
 public:
  bool LoadDics(const string filename_prefix);
  bool LoadTries(const string filename_prefix);
  bool LoadNgrams(const string filename_prefix);
  bool LoadVerbs(const string filename_prefix);
  bool LoadTekei(const string filename_prefix);
  bool IsTekei(uint32_t token_id, uint32_t* normalized_id, uint8_t* code) const;
  bool AdjustTekei(Pair* p) const;
  bool GetNgram(int n, uint32_t token_id, uint32_t context_id,
		uint32_t* new_context_id,
		NgramData* ngram_data) const;
  bool GetTokenId(const string key, uint32_t* token_id) const;
  bool GetPairs(const string src, size_t pos, vector<Pair>* results) const;
  bool GetVerbs(const string src, size_t pos,
		vector<vector<Pair> >* results) const;
  void GetPairStringFromId(uint32_t token_id, string* str);

 private:
  void GetUnigram(int token_id, NgramData* ngram) const;
  marisa::Trie trie_key_;
  marisa::Trie trie_pair_;
  marisa::Trie trie_verb_;
  scoped_array<uint8_t> ngram_data_;
  scoped_array<UnigramIndex> unigrams_;
  scoped_array<NgramIndex> ngram_indices_[MAX_N];
  scoped_array<uint8_t> filters_[MAX_N];
  scoped_array<uint32_t> verb_keys_;
  size_t verb_key_count_;
  scoped_array<VerbData> verb_data_;
  scoped_array<TekeiData> tekei_data_;
  size_t tekei_data_count_;
  uint8_t tekei_sei_code_;
  uint8_t tekei_daku_code_;
  size_t ngram_counts_[MAX_N];
  mutable NgramData ngram_data_work_[BLOCK_SIZE];
  int n_;
};

}  // namespace NgramConverter
#endif  // NGRAM_CONVERTER_LM_H_INCLUDED_
