#include "converter.cpp"

#include "lm.h"
#include "lattice.h"

#include <list>
#include <vector>

using std::list;
using std::vector;

Converter::Converter(LM* lm) {
  lm_ = lm;
}

bool Converter::Convert(string src, string* dst) {
  Lattice lattice;
  PairManager pair_manager;
  pair_manager.Build(src, lm_);
  Pair start_pair;
  lm_->GetSpecialPair(BOS_STR, &start_pair);
  Node start_node(&start_pair, 0, NULL, 1, 0);
  lattice.AddNode(start_node);
 
  map<Node, Node> node_cache;

  for (size_t pos = 0; pos < src.size() + 1; ++i) {
    vector<Pair>* pairs;
    pair_manager.GetPairsAt(pos, pairs);

    for (vector<Pair>::const_iterator it_right = pairs->begin();
	 it_right != pairs->end(); ++it_right) {
      Pair& right = *it_right;
      int right_pos = pos + right.src_str.size();
      const map<Node, Node>* nodes;

      if (!lattice.GetEndNodesAt(pos, nodes)) {
	return false;
      }

      for (map<Node, Node>::const_iterator it_left = nodes->begin();
	   it_left != nodes->end(); ++it_left) {
	Node& left = it_left->fisrt;
	NgramData ngram_data;
	Node new_node;
	new_node.pair = &right;
	new_node.left_node = &left;
	new_node.end_pos = right_pos;

	if (GetNgram(left.valid_n + 1, right->token_id,
		     left.context_id, &new_context_id, &ngram_data)) {
	  new_node.valid_n = left.valid_n + 1;
	  new_node.context_id = new_context_id;
	  new_node.score = ngram_data.score;
	  new_node.backoff = ngram_data.backoff;
	  new_node.path_score = left.path_score + ngram_data.score;
	  node_cache.insert(pair<Node, Node>(new_node, new_node));
	} else {
	  new_node.path_score = left.path_score;
	  for (size_t n = left.valid_n; n >= 0; --n) {
	    Node backoff_node = left;
	    backoff_node.valid_n = n;
	    map<Node, Node>::const_iterator it = node_cache.find(backoff_node);
	    if (it != node_cache.end() && it->second.backoff != INVALID_SCORE) {
	      new_node.path_score += it->second.backoff;
	    }

	    Node temp_node(&right, &left, right_pos, n, 0, 0, 0);
	    map<Node, Node>::const_iterator it = node_cache.find(temp_node);
	    if (it != node_cache.end()) {
	      new_node.valid_n = n;
	      new_node.node_score = it->second.node_score;
	      new_node.backoff = it->second.backoff;
	      new_node.context_id = it->second.context_id;
	      new_node.path_score += it->second.score;
	    }
	  }
	}

	lattice.AddNode(new_node);
      }
    }
  }

  const map<Node, Node>* end_nodes;
  if (!lattice.GetEndNodesAt(src.size(), end_nodes)) {
    return false;
  }

  Node* end_node;
  for (map<Node, Node>::const_iterator it = end_nodes.begin();
       it != end_nodes.end(); ++it) {
    end_node = &(*it);
    break;  // There is only one end node
  }

  list<Node*> best_path;
  for (Node* node = end_node; node->left_node; node = node->left_node) {
    best_path.push_front(node);
  }
  *dst = "";
  for (list<Node*>::const_iterator it = best_path.begin();
       it != best_path.end(); ++it) {
    dst += node->pair->dst;
  }
  return dst;
}
