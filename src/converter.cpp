#include <list>
#include <vector>

#include "converter.h"
#include "lm.h"
#include "lattice.h"

using std::list;
using std::map;
using std::vector;

namespace NgramConverter {

Converter::Converter(LM* lm) {
  lm_ = lm;
}

bool Converter::Convert(string src, string* dst) {
  Lattice lattice(src.size() + 1);
  PairManager pair_manager;
  if (!pair_manager.Build(src, *lm_)) {
    return false;
  }
  Pair bos_pair;
  uint32_t bos_token_id;
  if (!lm_->GetTokenId(BOS_STR, &bos_token_id)) {
    return false;
  }
  bos_pair.token_id = bos_token_id;
  bos_pair.src_str = bos_pair.dst_str = "";

  map<uint32_t, map<Node, Node> > node_cache;

  Node start_node;
  start_node.pair = &bos_pair;
  start_node.left_node = NULL;
  start_node.end_pos = 0;
  start_node.context_id = 0;
  start_node.valid_n = 1;
  start_node.path_score = 0;

  uint32_t new_context_id;
  NgramData ngram_data;
  if (!lm_->GetNgram(1, bos_pair.token_id, 0, &new_context_id, &ngram_data)) {
    return false;
  }
  
  start_node.node_score = ngram_data.score;
  start_node.backoff = ngram_data.backoff;
  start_node.context_id = new_context_id;

  if (!lattice.AddNode(start_node)) {
    return false;
  }
  node_cache[0].insert(pair<Node, Node>(start_node, start_node));

  const map<uint32_t, vector<Pair> >& pos_map = pair_manager.GetMapRef();

  for (map<uint32_t, vector<Pair> >::const_iterator it_pos = pos_map.begin();
       it_pos != pos_map.end(); ++it_pos) {
    uint32_t pos = it_pos->first;
    const vector<Pair>& pairs = it_pos->second;

    if (pairs.size()) {
      Node zero_length_node;
      zero_length_node.end_pos = pos;
      zero_length_node.valid_n = 0;
      zero_length_node.backoff = 0;
      zero_length_node.path_score = INVALID_SCORE;
      if (!lattice.AddNode(zero_length_node)) {
	return false;
      }
      node_cache[pos].insert(pair<Node, Node>(zero_length_node, zero_length_node));
    } else {
      continue;
    }

    for (vector<Pair>::const_iterator it_right = pairs.begin();
	 it_right != pairs.end(); ++it_right) {
      const Pair& right = *it_right;
      int right_pos = pos + right.length;
      const map<Node, Node>* nodes;

      lattice.GetEndNodesAt(pos, &nodes);

      for (map<Node, Node>::const_iterator it_left = nodes->begin();
	   it_left != nodes->end(); ++it_left) {
	const Node& left = it_left->second;
	NgramData ngram_data;
	Node new_node;
	new_node.pair = &right;
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
	    temp_node.pair = &right;
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
  lattice.GetEndNodesAt(src.size() * MAX_INFLECTION + 1, &end_nodes);

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
    string str = (*it)->pair->dst_str;
    if (str.empty()) {
      lm_->GetPairStringFromId((*it)->pair->token_id, &str);
    }
    *dst += str;
    *dst += " ";
  }
  return true;
}

}  // namespace NgramConverter
