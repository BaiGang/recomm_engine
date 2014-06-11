#include <iostream>
#include "./str_util.h"

using std::cout;
using std::endl;

int main(int argc, char** argv) {
	std::string s;
	util::StringAppendF(&s, "%d %d", 1, 2);
	util::StringAppendF(&s, " %d %d", 1, 2);
	cout << s << endl;
  return 0;
}
