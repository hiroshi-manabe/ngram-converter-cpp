#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <sstream>

#include "lattice.h"
#include "lm.h"
#include "pair.h"

using std::cout;
using std::endl;
using std::stringstream;

namespace NgramConverter {

uint32_t Node::GetTokenId() const {
  return pair->token_id;
}

bool Node::operator<(const Node& another) const {
  int result;

  result = this->end_pos - another.end_pos;
  if (result < 0) {
    return true;
  } else if (result > 0) {
    return false;
  }

  result = (another.valid_n == 0) - (this->valid_n == 0);
  if (result < 0) {
    return true;
  } else if (result > 0) {
    return false;
  }

  const Node* this_left = this;
  const Node* another_left = &another;

  for (int i = 0; i < std::min(this->valid_n, another.valid_n); ++i) {
    result = (this_left->GetTokenId() - another_left->GetTokenId());

    if (result < 0) {
      return true;
    } else if (result > 0) {
      return false;
    }

    this_left = this_left->left_node;
    another_left = another_left->left_node;
  }
  result = this->valid_n - another.valid_n;
  if (result < 0) {
    return true;
  }
  return false;
}

string Node::d() const {
  stringstream ss;
  const Node* left = this;

  for (int i = 0; i < valid_n; ++i) {
    ss << "[" << left->pair->src_str << "/" << left->pair->dst_str << "("
       << left->pair->token_id << ")]";
    left = left->left_node;
  }
  ss << "(c:" << context_id << ",ns:" << node_score << ",b:" << backoff
     << ",p:" << path_score << ")";
  return ss.str();
}

Lattice::Lattice(size_t len) {
  end_nodes_.resize(len + 1);
}

bool Lattice::AddNode(Node node) {
  map<Node, Node>& map_to_add = end_nodes_[node.end_pos];

  map<Node, Node>::const_iterator it;
  if (node.backoff != INVALID_SCORE) {
    it = map_to_add.find(node);
  } else {
    for (; node.valid_n >= 0; --node.valid_n) {
      it = map_to_add.find(node);
      if (it != map_to_add.end()) {
	break;
      }
    }
  }

  if (it == map_to_add.end()) {
    //    cout << node.d() << endl;
    map_to_add[node] = node;
  } else if (node.path_score > it->second.path_score) {
    //    cout << node.d() << endl;
    const Node& old = it->second;
    node.node_score = old.node_score;
    node.backoff = old.backoff;
    node.context_id = old.context_id;
    map_to_add[node] = node;
  }
  return true;
}

void Lattice::GetEndNodesAt(int pos, const map<Node, Node>** nodes) {
  *nodes = &end_nodes_[pos];
}

}  // namespace NgramConverter
