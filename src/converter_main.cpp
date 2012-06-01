#include "lm.h"

void Usage() {
  printf("Usage: converter_main -i <input filename prefix>");
  exit(0);
}

int main(int argc, char* argv[]) {
  string input_filename_prefix;

  for (int i = 1; i < argc; ++i) {
    if (string(argv[i]) == "-i") {
      input_filename_prefix = argv[i+1];
      ++i;
    }
  }

  if (input_filename_prefix == "") {
    Usage();
  }

  NgramConverter::LM lm;
  lm.LoadDics(input_filename_prefix);

  return 0;
}
