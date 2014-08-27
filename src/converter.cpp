#include <list>
#include <utility>
#include <vector>

#include "converter.h"
#include "lm.h"
#include "lattice.h"

using std::list;
using std::pair;
using std::vector;

namespace NgramConverter {

Converter::Converter(LM* lm) {
  lm_ = lm;
}

bool Converter::Convert(string src, string* dst) {
  Lattice lattice(src.size() + 1);
  WordManager word_manager;
  if (!word_manager.Build(src, *lm_)) {
    return false;
  }
  Word bos_word;
  uint32_t bos_token_id;
  if (!lm_->GetTokenId(BOS_STR, &bos_token_id)) {
    return false;
  }
  bos_word.token_id = bos_token_id;
  bos_word.src_str = bos_word.dst_str = "";

  vector<map<Node, Node> > node_cache;
  node_cache.resize(src.size() + 2);  // the end pos of EOS is size+1

  Node start_node;
  start_node.word = &bos_word;
  start_node.left_node = NULL;
  start_node.end_pos = 0;
  start_node.context_id = 0;
  start_node.valid_n = 1;
  start_node.path_score = 0;

  uint32_t new_context_id;
  NgramData ngram_data;
  if (!lm_->GetNgram(1, bos_word.token_id, 0, &new_context_id, &ngram_data)) {
    return false;
  }
  
  start_node.node_score = ngram_data.score;
  start_node.backoff = ngram_data.backoff;
  start_node.context_id = new_context_id;

  if (!lattice.AddNode(start_node)) {
    return false;
  }
  node_cache[0].insert(pair<Node, Node>(start_node, start_node));

  for (size_t pos = 0; pos <= src.size(); ++pos) {
    const vector<Word>* words;
    word_manager.GetWordsAt(pos, &words);

    if (words->size()) {
      Node zero_length_node = {};
      zero_length_node.end_pos = pos;
      zero_length_node.path_score = INVALID_SCORE;

      if (!lattice.AddNode(zero_length_node)) {
	return false;
      }
      node_cache[pos].insert(pair<Node, Node>(zero_length_node, zero_length_node));
    } else {
      continue;
    }

    for (vector<Word>::const_iterator it_right = words->begin();
	 it_right != words->end(); ++it_right) {
      const Word& right = *it_right;
      int right_pos = pos + right.src_str.size();
      const map<Node, Node>* nodes;

      lattice.GetEndNodesAt(pos, &nodes);

      for (map<Node, Node>::const_iterator it_left = nodes->begin();
	   it_left != nodes->end(); ++it_left) {
	const Node& left = it_left->second;
	NgramData ngram_data;
	Node new_node = {};
	new_node.word = &right;
	new_node.left_node = &left;
	new_node.end_pos = right_pos;

	if (lm_->GetNgram(left.valid_n + 1, right.token_id,
			  left.context_id, &new_context_id, &ngram_data)) {
	  new_node.valid_n = left.valid_n + 1;
	  new_node.context_id = new_context_id;
	  new_node.node_score = ngram_data.score;
	  new_node.backoff = ngram_data.backoff;
	  new_node.path_score = left.path_score + ngram_data.score;
	  node_cache[new_node.end_pos].insert(pair<Node, Node>(new_node,
							       new_node));
	} else {
	  new_node.path_score = left.path_score;
	  for (int n = left.valid_n; n >= 0; --n) {
	    Node backoff_node = left;
	    backoff_node.valid_n = n;
	    map<Node, Node>::const_iterator it;
	    it = node_cache[pos].find(backoff_node);
	    if (it != node_cache[pos].end() &&
		it->second.backoff != INVALID_SCORE) {
	      new_node.path_score += it->second.backoff;
	    }

	    Node temp_node;
	    temp_node.word = &right;
	    temp_node.left_node = &left;
	    temp_node.end_pos = right_pos;
	    temp_node.valid_n = n;
	    it = node_cache[temp_node.end_pos].find(temp_node);
	    if (it != node_cache[temp_node.end_pos].end()) {
	      new_node.valid_n = n;
	      new_node.node_score = it->second.node_score;
	      new_node.backoff = it->second.backoff;
	      new_node.context_id = it->second.context_id;
	      new_node.path_score += it->second.node_score;
	      break;
	    }
	  }
	}
	if (!lattice.AddNode(new_node)) {
	  return false;
	}
      }
    }
  }

  const map<Node, Node>* end_nodes;
  lattice.GetEndNodesAt(src.size() + 1, &end_nodes);

  const Node* end_node = NULL;
  for (map<Node, Node>::const_iterator it = end_nodes->begin();
       it != end_nodes->end(); ++it) {
    if (!end_node || it->second.path_score > end_node->path_score) {
      end_node = &(it->second);
    }
  }

  list<const Node*> best_path;
  for (const Node* node = end_node; node->left_node; node = node->left_node) {
    best_path.push_front(node);
  }
  *dst = "";
  for (list<const Node*>::const_iterator it = best_path.begin();
       it != best_path.end(); ++it) {
    *dst += (*it)->word->dst_str;
  }
  return true;
}

}  // namespace NgramConverter
