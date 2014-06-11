// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Test for index building
#include <set>
#include <vector>
#include <string>
#include <stdio.h>
#include "recomm_engine_types.h"
#include "src/indexing/instant_index.h"
#include "./gtest.h"

using std::vector;
using std::set;
using std::string;

using namespace recomm_engine::indexing;

namespace recomm_engine {
namespace indexing {
const int NUM_DOCS = 100;
const int EXP_NUM_WORDS = 100;
class IndexBuilderTest : public ::testing::Test {
  public:
	  void SetUp();
		void TearDown();
		InstantIndex* index;
    vector<set<int> > docs;
};
void IndexBuilderTest::SetUp() {
  index = InstantIndex::GetInstance();
	// fake docs

  for (int i = 0; i < NUM_DOCS; i++) {
		docs.push_back(set<int>());
		for (int j = 0; j < EXP_NUM_WORDS; j++) {
		  int word_id = rand() % EXP_NUM_WORDS;
			docs[i].insert(word_id);
		}
	}
}
void IndexBuilderTest::TearDown() {
  delete index;
}
TEST_F(IndexBuilderTest, TestReplayLog) {
	LOG(INFO) << index;
	InstantIndex* null_p = NULL;
  EXPECT_NE(index, null_p);
	LOG(INFO) << "initializing ";
	// insert docs
	for (int i = 0; i < 10; i++) {
	  idl::StoryAddingRequest req;
		char buf[10];
		sprintf(buf, "%d", i+1);
		req.story.story_id = string(buf);
		for (set<int>::iterator it = docs[i].begin();
		     it != docs[i].end(); ++it) {
		  req.story.keywords[*it] = 1;
	  }
		req.story.__isset.keywords = true;
		sleep(1);
		index->AddStory(req);
		LOG(INFO) << "doc added " << req.story.story_id;
	}

	sleep(50000);
}
}
}
