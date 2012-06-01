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
  Node(const Pair* pair, const Node* left_node, size_t valid_n, double score) :
  pair(pair), left_node(left_node), valid_n(valid_n) {};
  uint32_t GetTokenId() const;
  bool operator<(const Node& another) const;
  int GetEndPos() const;

  const Pair* pair;
  const Node* left_node;
  size_t valid_n;
  double node_score;
  double node_backoff;
  double path_score;

 private:
  int32_t GetTokenId();
};

class Lattice {
 public:
  bool AddNode(Node node);
  const map<Node, Node>& GetEndNodesAt(int pos, bool& found);

 private:
  list<pair<map<Node, Node>, int> > end_nodes_;
};

}  // namespace NgramConverter

#endif  // NGRAM_CONVERTER_LATTICE_H_INCLUDED_
