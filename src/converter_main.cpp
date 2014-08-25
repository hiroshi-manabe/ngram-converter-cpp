#include "converter.h"
#include "lm.h"

#include <cstdlib>
#include <iostream>

using std::cin;
using std::cout;
using std::endl;

void Usage() {
  printf("Usage: converter_main -i <input filename prefix>\n");
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
  NgramConverter::Converter converter(&lm);

  string src;
  string dst;

  while (cin >> src) {
    if (!converter.Convert(src, &dst)) {
      cout << "An error occured." << endl;
      break;
    }
    cout << dst << endl;
  }

  return 0;
}
