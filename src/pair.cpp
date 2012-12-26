#include <utility>

#include "lm.h"
#include "pair.h"

using std::pair;

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

template <typename K, typename V>
V setdefault(const  std::map <K,V>& m, const K& key, const V& defval ) {
  typename std::map<K,V>::const_iterator it = m.find(key);
  if (it == m.end()) {
    return defval;
  }
  else {
    return it->second;
  }
}

bool PairManager::Build(const string src, const LM& lm) {
  for (size_t pos = 0; pos < src.size(); pos = NextUtf8Pos(src, pos)) {
    vector<Pair> results;

    if (!lm.GetPairs(src, pos, &results)) {
      return false;
    }
    for (vector<Pair>::const_iterator it = results.begin();
	 it != results.end(); ++it) {
      pos_map_[it->start_pos].push_back(*it);
    }
    if (results.size() == 0) {
      size_t next_pos = NextUtf8Pos(src, pos);
      Pair unknown_pair;
      uint32_t unk_token_id;
      if (!lm.GetTokenId(UNK_STR, &unk_token_id)) {
	//	return false;
	unk_token_id = 0;
      }
      unknown_pair.src_str = src.substr(pos, next_pos - pos);
      unknown_pair.dst_str = unknown_pair.src_str;
      unknown_pair.token_id = unk_token_id;
      unknown_pair.length = unknown_pair.src_str.size() * MAX_INFLECTION; 
      
      pos_map_[pos * MAX_INFLECTION].push_back(unknown_pair);
    }

    vector<vector<PairInflection> > results_verb;
    if (!lm.GetVerbs(src.substr(pos), &results_verb)) {
      return false;
    }
    for (vector<vector<PairInflection> >::const_iterator it =
	   results_verb.begin();
	 it != results_verb.end(); ++it) {
      const vector<PairInflection>& v = *it;
      uint32_t temp_pos = pos * MAX_INFLECTION;
      for (vector<PairInflection>::const_iterator it2 =v.begin();
	   it2 != v.end(); ++it2) {
	Pair pair = it2->pair;
	pos_map_[temp_pos].push_back(it2->pair);
	temp_pos += it2->pair.length;
      }
    }
  }

  Pair eos_pair;
  uint32_t eos_token_id;
  if (!lm.GetTokenId(EOS_STR, &eos_token_id)) {
    return false;
  }
  eos_pair.token_id = eos_token_id;
  eos_pair.src_str = " ";
  eos_pair.dst_str = "";
  eos_pair.length = 1;
  pos_map_[src.size() * MAX_INFLECTION].push_back(eos_pair);
  return true;
}

PairInflection::PairInflection(uint32_t code, uint32_t prev_code,
			       uint32_t length) {
  pair.src_str = "";
  pair.dst_str = "";
  pair.token_id = code & 0x00FFFFFF;
  pair.length = 0;
  inflection_code = ((code & 0xFF000000) >> 24);

  uint8_t prev_inflection_code = 0;
  if (prev_code != INVALID_CODE) {
    prev_inflection_code = ((prev_code & 0xFF000000) >> 24);
  } else {
    pair.length = (length - 1) * MAX_INFLECTION;
  }

  if (inflection_code) {
    pair.length += inflection_code - prev_inflection_code;
  } else {
    pair.length += MAX_INFLECTION - prev_inflection_code;      
  }
}


const map<uint32_t, vector<Pair> >& PairManager::GetMapRef() const {
  return pos_map_;
}

}  // namespace NgramConverter
