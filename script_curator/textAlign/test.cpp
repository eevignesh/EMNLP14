#include <boost/regex.hpp>
#include <iostream>
#include <string.h>
using namespace std;

int main() {

  boost::regex re("CONTINUED:?\\s+\\(?\\d+\\)?");

  string s = "CONTINUED: (2)";

  string s_out = boost::regex_replace(s, re, "-");
  cout << "ORIGINAL: " << s << endl << "CLEAN: " << s_out << endl;

  return 0;

}

