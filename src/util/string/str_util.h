// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Description: Process string

#ifndef SRC_UTIL_STRING_STR_UTIL_H_
#define SRC_UTIL_STRING_STR_UTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

namespace util {
const int STR_BUF_SIZE = 1024;
bool StringAppendF(std::string* dst, const char* format, ...);
void SplitString(const std::string& str, char del,
                 std::vector<std::string>* segs);
};
#endif  // SRC_UTIL_STRING_STR_UTIL_H_
