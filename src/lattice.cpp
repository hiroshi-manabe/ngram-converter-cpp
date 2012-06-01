#include <algorithm>
#include <stdexcept>

#include "lattice.h"
#include "pair.h"

namespace NgramConverter {

uint32_t Node::GetTokenId() const {
  return pair->token_id;
}

bool Node::operator<(const Node& another) const {
  const Node* this_left = this;
  const Node* another_left = &another;

  for (size_t i = 0; i < std::min(this->valid_n, another.valid_n); ++i) {
    int result = (this_left->GetTokenId() - another_left->GetTokenId());
    if (result) {
      return result;
    }
    this_left = this_left->left_node;
    another_left = another_left->left_node;
  }
  return this->valid_n - another.valid_n;
}

int Node::GetEndPos() const {
  return pair->token_id;
}

bool Lattice::AddNode(Node node) {
  bool push_back_new = false;
  int node_end_pos = 0;

  if (!end_nodes_.empty()) {
    int last_end_pos = end_nodes_.back().second;
    int node_end_pos = node.GetEndPos();

    if (last_end_pos > node_end_pos) {
      return false;
    } else if (last_end_pos < node_end_pos) {
      push_back_new = true;
    }
  } else {  // end_nodes_ is empty
    push_back_new = true;
  }
  if (push_back_new) {
    map<Node, Node> last_map;
    end_nodes_.push_back(pair<map<Node, Node>, int>(last_map, node_end_pos));
  }
  end_nodes_.push_back(pair<map<Node, Node>, int>());

  map<Node, Node>& last_map = end_nodes_.back().first;
  map<Node, Node>::const_iterator it = last_map.find(node);

  if (it == last_map.end() || node.path_score > it->second.path_score) {
    last_map.insert(pair<Node, Node>(node, node));
  }
  return true;
}

const map<Node, Node>& Lattice::GetEndNodesAt(int pos, bool& found) {
  found = false;
  for (list<pair<map<Node, Node>, int> >::const_reverse_iterator it
	 = end_nodes_.rbegin(); it != end_nodes_.rend(); ++it) {
    if (it->second == pos) {
      found = true;
      return it->first;
    }
  }
  throw std::out_of_range("Not found");
}

}  // namespace NgramConverter
