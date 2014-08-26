#include "util.h"
#include <cstring>

namespace NgramConverter {

using std::string;

void UnescapeString(const std::string& src_str, std::string* dst_str)
{
  dst_str->clear();
  for (string::const_iterator it = src_str.begin();
       it != src_str.end(); ++it) {
    if (*it == '\\') {
      ++it;
      if (it == src_str.end()) {
	break;
      }
    }
    dst_str.append(*it);
  }
}

void EscapeString(const std::string& src_str, std::string* dst_str, const char* escape_chars)
{
  dst_str->clear();
  for (string::const_iterator it = src_str.begin();
       it != src_str.end(); ++it) {
    if (strchr(escape_chars, *it) != 0) {
      dst_str.append('\\');
    }
    dst_str.append(*it);
  }
}

}
