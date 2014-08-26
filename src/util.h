#ifndef NGRAM_CONVERTER_UTL_H_INCLUDED_
#define NGRAM_CONVERTER_UTIL_H_INCLUDED_

#include <string>

namespace NgramConverter {

void EscapeString(const std::string& src_str, std::string* dst_str);
void UnescapeString(const std::string& src_str, std::string* dst_str, const char* escape_chars = "\\/");

}  // namespace NgramConverter

#endif  // NGRAM_CONVERTER_UTIL_H_INCLUDED_

