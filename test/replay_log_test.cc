// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Description: Test for replay log

#include <fstream>
#include "gtest.h"
#include "glog/logging.h"
#include "util/base/timer.h"
#include "recomm_engine_types.h"
#include "util/base/thrift_util.h"

namespace recomm_engine{
namespace indexing {

using idl::StoryAddingRequest;
using util::ThriftToString;
using util::ThriftToDebugString;
using util::StringToThrift;
using std::string;

class ReplayLogTest : public ::testing::Test {
  public:
	  void SetUp();
		void TearDown();
};
void ReplayLogTest::SetUp() {
}
void ReplayLogTest::TearDown() {
}
TEST_F(ReplayLogTest, rw_log) {
  StoryAddingRequest req;
	req.story.story_id = "abcdefg";
	for (int i = 0;i < 5; i++) {
	  req.story.keywords[i] = 1;
	}
	req.story.__isset.keywords = true;
	std::ofstream out("/tmp/test_log");
	string str = util::ThriftToString(&req);
	int now = util::Timer::CurrentTimeInS();
	int size = str.size();
	out.write(reinterpret_cast<char*>(&now), sizeof(int));
	out.write(reinterpret_cast<char*>(&size), sizeof(int));
	out.write(str.c_str(), size);
	out.close();
	//
	std::ifstream in("/tmp/test_log");
	in.read(reinterpret_cast<char*>(&now), sizeof(int));
	in.read(reinterpret_cast<char*>(&size), sizeof(int));
	char buf[1024];
	in.read(buf, size);
	LOG(INFO) << now;
	LOG(INFO) << size;
	string ss(buf, size);
	StoryAddingRequest req2;
	LOG(INFO) << StringToThrift(ss, &req2);
	LOG(INFO) << "Dump:" << ThriftToDebugString(&req2);
}
}
}
