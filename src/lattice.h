#ifndef NGRAM_CONVERTER_LATTICE_H_INCLUDED_
#define NGRAM_CONVERTER_LATTICE_H_INCLUDED_

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

using std::string;
using std::map;
using std::vector;

namespace NgramConverter {

struct Word;

struct Node {
  uint32_t GetTokenId() const;
  bool operator<(const Node& another) const;

  const Word* word;
  const Node* left_node;

  int end_pos;
  uint32_t context_id;
  int valid_n;
  double node_score;
  double backoff;
  double path_score;

  string d() const;
};

class Lattice {
 public:
  Lattice(size_t len);
  bool AddNode(Node node);
  void GetEndNodesAt(int pos, const map<Node, Node>** nodes);

 private:
  vector<map<Node, Node> > end_nodes_;
};

}  // namespace NgramConverter

#endif  // NGRAM_CONVERTER_LATTICE_H_INCLUDED_
