// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Test for str util
#include <string>
#include <vector>
#include "util/string/str_util.h"
#include "./gtest.h"
#include "glog/logging.h"

using std::vector;
using std::string;

namespace util {
class StringUtilTest : public ::testing::Test {
  public:
	  void SetUp();
		void TearDown();
};
void StringUtilTest::SetUp() {
  LOG(INFO) << "Setting up";
}
void StringUtilTest::TearDown() {
  LOG(INFO) << "Tearing down";
}
TEST_F(StringUtilTest, TestSplitString) {
  string str = "a_b_c";
	char del = '_';
	vector<string> segs;
	SplitString(str, del, &segs);
  EXPECT_EQ(segs.size(), static_cast<size_t>(3));
	EXPECT_EQ(segs[0], "a");
	EXPECT_EQ(segs[1], "b");
	EXPECT_EQ(segs[2], "c");

	str = "_";
	segs.clear();
	SplitString(str, del, &segs);
  EXPECT_EQ(segs.size(), static_cast<size_t>(0));

	str = "_a_b_c";
	segs.clear();
	SplitString(str, del, &segs);
	EXPECT_EQ(segs.size(), 3u);

	str = "a_b_dec_";
	segs.clear();
	SplitString(str, del, &segs);
	EXPECT_EQ(segs.size(), 3u);

	str = "_a_b_dec_";
	segs.clear();
	SplitString(str, del, &segs);
	EXPECT_EQ(segs.size(), 3u);
	LOG(INFO) << segs[0];
	LOG(INFO) << segs[1];
	LOG(INFO) << segs[2];

	str = "___";
	segs.clear();
	SplitString(str, del, &segs);
	EXPECT_EQ(segs.size(), 0u);
}
}   // namespace util

