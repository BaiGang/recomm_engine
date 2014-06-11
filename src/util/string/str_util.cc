// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Description: Process string

#include <stdarg.h>
#include "./str_util.h"

using std::string;
using std::vector;

namespace util {
void SplitString(const string& str, char del, vector<string>* segs) {
  if (str.empty()) return;
	size_t last_pos = 0;
	size_t cur_pos = 0;
	do {
		string seg;
	  cur_pos = str.find(del, last_pos);
		size_t end_pos = std::min(cur_pos, str.size());
		for (size_t i = last_pos; i < end_pos; i++) {
		  seg.push_back(str[i]);
		}
		if (!seg.empty()) {
		  segs->push_back(seg);
		}
		last_pos = std::min(cur_pos, str.size()) + 1;
	} while(last_pos < str.size());
}
bool StringAppendF(string* dst, const char* format, ...) {
  va_list arguments;
	char buf[STR_BUF_SIZE];
	va_start(arguments, format);
	int cnt = vsnprintf(buf, STR_BUF_SIZE, format,arguments);
	va_end(arguments);
	dst->append(buf);
	if (cnt < STR_BUF_SIZE) {
	  return true;
	} else {
	  return false;
	}
}
};
