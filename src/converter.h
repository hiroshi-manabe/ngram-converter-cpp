#ifndef NGRAM_CONVERTER_CONVERTER_H_INCLUDED_
#define NGRAM_CONVERTER_CONVERTER_H_INCLUDED_

#include <string>

using std::string;

namespace NgramConverter {

class LM;

class Converter {
 public:
  Converter(LM* lm);
  bool Convert(const string &src, string *dst);

 private:
  LM* lm_;
};

}  // namespace NgramConverter

#endif  // NGRAM_CONVERTER_CONVERTER_H_INCLUDED_
