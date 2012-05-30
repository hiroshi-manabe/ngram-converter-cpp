#include "lm.h"

#include <cstdio>

namespace {

inline double DecodeScore(int x) {
  return static_cast<double>(x) * MIN_SCORE / MAX_INT_SCORE;
}

inline double DecodeBackoff(int x) {
  return static_cast<double>(x - BACKOFF_DISCOUNT) * MIN_SCORE / MAX_INT_SCORE;
}

size_t DecodeNumber(const uint8_t* p, uint32_t* x) {  
  size_t len = 0;
  if (p[0] & (1 << 7)) {
    *x = p[0] & ((1 << 7) - 1);
    len = 1;
  } else if (p[0] & (1 << 6)) {
    *x = ((p[0] & ((1 << 6) - 1)) << 8) | p[1];
    len = 2;
  } else if (p[0] & (1 << 5)) {
    *x = ((p[0] & ((1 << 5) - 1)) << 16) | (p[1] << 8) | p[2];
    len = 3;
  } else if (p[0] & (1 << 4)) {
    *x = ((p[0] & ((1 << 4) - 1)) << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    len = 4;
  }
  return len;
}

size_t BlockCount(size_t n) {
  return ((n + BLOCK_SIZE - 1) / BLOCK_SIZE);
}

bool DecodeBlock(const uint8_t* buf, size_t buf_size,
		 uint32_t first_token_id, uint32_t first_context_id,
                 NgramConverter::NgramData* ngrams) {
  size_t offset = 0;
  uint8_t flags = buf[offset];
  ++offset;

  for (int i = 0; i < BLOCK_SIZE; ++i) {
    ngrams[i].score = DecodeScore(buf[offset]);
    ++offset;
  }

  if (flags & FLAGS_HAS_BACKOFF) {
    size_t offset_bit_array = offset;
    offset += BLOCK_SIZE / 8;
    for (int i = 0; i < BLOCK_SIZE; ++i) {
      if (buf[offset_bit_array + i / 8] & (1 << (i % 8))) {
	ngrams[i].backoff = DecodeBackoff(buf[offset]);
	++offset;
      } else {
	ngrams[i].backoff = INVALID_SCORE;
      }
    }
  }

  if (flags & FLAGS_HAS_TOKEN_IDS) {
    size_t bit_offset = 0;
    ngrams[0].token_id = first_token_id;
    for (int i = 1; i < BLOCK_SIZE; ++i) {
      size_t zero_count = 0;
      while (!(buf[offset + bit_offset / 8] & (1 << (bit_offset % 8)))) {
	zero_count++;
	bit_offset++;
      }
      ngrams[i].token_id = ngrams[i-1].token_id + zero_count;
    }
    offset += int((bit_offset + 7) / 8);
  } else {  // flags & FLAGS_HAS_TOKEN_IDS
    for (int i = 0; i < BLOCK_SIZE; ++i) {
      ngrams[i].token_id = first_token_id;
    }
  }

  ngrams[0].context_id = first_context_id;
  for (int i = 1; i < BLOCK_SIZE; ++i) {
    uint32_t context_id_offset;
    size_t len = DecodeNumber(buf + offset, &context_id_offset);
    ngrams[i].context_id = context_id_offset;
    if (ngrams[i].token_id == ngrams[i-1].token_id) {
      ngrams[i].context_id += ngrams[i-1].context_id;
    }
    offset += len;
  }

  if (offset != buf_size) {
    return false;
  }

  return true;
}

}  // namespace

namespace NgramConverter {

bool LM::LoadDics(const string filename_prefix) {
  if (!LoadTrie(filename_prefix + EXT_TRIE)) {
    return false;
  }
  if (!LoadNgrams(filename_prefix)) {
    return false;
  }
  return true;
}

bool LM::LoadTries(const string filename_prefix) {
  trie_key_.load((filename_prefix + EXT_KEY).c_str());
  trie_pair_.load((filename_prefix + EXT_PAIR).c_str());
  return true;
}

bool LM::LoadNgrams(const string filename_prefix) {
  bool is_success = false;
  size_t size_read;
  size_t size_to_read;
  int cur_n = 1;

  uint32_t unigram_count;
  uint32_t data_size;
  
  FILE* fp_index = fopen((filename_prefix + EXT_INDEX).c_str(), "rb");
  FILE* fp_data = fopen((filename_prefix + EXT_DATA).c_str(), "rb");

  if (fseek(fp_data, 0, SEEK_END) != 0) {
    goto LOAD_NGRAMS_END;
  }

  data_size = ftell(fp_data);
  ngram_data_.reset(new uint8_t[data_size]);

  if (fseek(fp_data, 0L, SEEK_SET) != 0) {
    goto LOAD_NGRAMS_END;
  }

  size_read = fread(ngram_data_.get(), 1, data_size, fp_data); 
  if (size_read < data_size) {
    goto LOAD_NGRAMS_END;
  }

  size_read = fread(&unigram_count, sizeof(uint32_t), 1, fp_index);
  if (size_read < 1) {
    goto LOAD_NGRAMS_END;
  }

  unigrams_.reset(new UnigramIndex[unigram_count]);
  ngram_counts_[1] = unigram_count;

  size_read = fread(unigrams_.get(), sizeof(UnigramIndex), unigram_count,
		    fp_index);
  if (size_read < unigram_count) {
    goto LOAD_NGRAMS_END;
  }

  while (1) {
    uint32_t ngram_count;
    size_read = fread(&ngram_count, sizeof(uint32_t), 1, fp_index);

    if (feof(fp_index)) {
      break;
    }
    ++cur_n;

    ngram_indices_[cur_n].reset(new NgramIndex[ngram_count]);
    size_to_read = BlockCount(ngram_count);
    size_read = fread(ngram_indices_[cur_n].get(), sizeof(NgramIndex),
		      size_to_read, fp_index);

    if (size_read < size_to_read) {
      goto LOAD_NGRAMS_END;
    }
    ngram_counts_[cur_n] = ngram_count;
  }
  n_ = cur_n;
  is_success = true;

 LOAD_NGRAMS_END:
  fclose(fp_index);
  fclose(fp_data);
  
  return is_success;
}

void LM::GetUnigram(int token_id, NgramData* ngram) {
  ngram->token_id = token_id;
  ngram->context_id = 0;
  ngram->score = DecodeScore(unigrams_[token_id].score_int);
  ngram->backoff = DecodeBackoff(unigrams_[token_id].backoff_int);
}

bool LM::GetNgram(int n, int token_id, int context_id, int* new_context_id,
		  NgramData* ngram) {
  if (n == 0 || n > n_) {
    return false;
  }

  if (n == 1) {
    GetUnigram(token_id, ngram);
    *new_context_id = token_id;
    return true;
  }
  // n > 1
  size_t block_count = BlockCount(ngram_counts_[n]);

  int block_num;
  for (block_num = 0; block_num < block_count; ++block_num) {
    if (ngram_indices_[n][block_num].token_id < token_id) {
      continue;
    }
    if (ngram_indices_[n][block_num].token_id > token_id ||
	ngram_indices_[n][block_num].context_id > context_id) {
      break;
    }
  }
  if (block_num == 0) {
    return false;
  }

  --block_num;
  size_t data_start = block_num > 0 ?
    ngram_indices_[n][block_num-1].data_offset
    : 0;
  size_t data_end = ngram_indices_[n][block_num].data_offset;
  
  if (!DecodeBlock(ngram_data_.get() + data_start, data_end - data_start,
		   ngram_indices_[n][block_num].token_id,
		   ngram_indices_[n][block_num].context_id,
		   ngram_data_work_)) {
    return false;
  }

  for (int i = 0; block_num * BLOCK_SIZE + i < ngram_counts_[n]; ++i) {
    if (ngram_data_work_[i].token_id == token_id &&
	ngram_data_work_[i].context_id == context_id) {
      *ngram = ngram_data_work_[i];
      *new_context_id = block_num * BLOCK_SIZE + i;
      return true;
    }
  }
  return false;
}

bool LM::GetPairs(const string src, vector<pair<string, string> >* pairs) {
  Marisa::Agent agent_key;
  agent_key.set_query(src.c_str());

  while (trie_key_.common_prefix_search(agent_key)) {
    Marisa::Agent agent_pair;
    string key_str(agent_key.key().ptr(), agent_key.key().length());

    key_str += PAIR_SEPARATOR;
    agent_pair.set_query(key_str.c_str(), key_str.size());

    while (trie_pair_.predictive_search(agent_pair)) {
      string pair_str(agent_pair.key().prt(), agent_pair.key().length());
      size_t pos = pair_str.find(PAIR_SEPARATOR);

      if (pos == string::npos) {
	return false;
      }

      string src = pair_str.substr(0, pos);
      string dst = pair_str.substr(pos);
      pairs->push_back(pair(src, dst));
    }
  }
  return true;
}

}  // namespace NgramConverter
