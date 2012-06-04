#ifndef NGRAM_CONVERTER_LATTICE_H_INCLUDED_
#define NGRAM_CONVERTER_LATTICE_H_INCLUDED_

#include <list>
#include <map>
#include <string>
#include <utility>

using std::string;
using std::list;
using std::map;
using std::pair;

namespace NgramConverter {

class Pair;

struct Node {
  uint32_t GetTokenId() const;
  bool operator<(const Node& another) const;

  const Pair* pair;
  const Node* left_node;

  int end_pos;
  uint32_t context_id;
  size_t valid_n;
  double node_score;
  double backoff;
  double path_score;
};

class Lattice {
 public:
  bool AddNode(Node node);
  bool GetEndNodesAt(int pos, const map<Node, Node>* nodes);

 private:
  list<pair<map<Node, Node>, int> > end_nodes_;
};

}  // namespace NgramConverter

#endif  // NGRAM_CONVERTER_LATTICE_H_INCLUDED_
