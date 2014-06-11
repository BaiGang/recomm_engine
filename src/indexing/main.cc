// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Only for testing

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_set.hpp>
#include <string>
#include <vector>
#include <map>
#include <iostream>  // NOLINT
#include "./instant_index.h"
#include "glog/logging.h"
#include "./posting.h"
#include "gflags/gflags.h"
#include "./recomm_engine_types.h"
#include "./iterator.h"
#include "util/base/timer.h"


using recomm_engine::idl::StoryProfile;
using recomm_engine::idl::StoryAddingRequest;
using std::vector;
using std::string;
using std::cout;
using std::endl;
using std::map;
using boost::shared_ptr;
using namespace recomm_engine::indexing;  // NOLINT
using util::Timer;

const int NUM_TOKEN = 1000;
const int NUM_DOCS = 50000;
const int NUM_KEYWORDS = 50;
unsigned int retrieval_counter = 0;
unsigned int latency = 0;

void ParseInput(vector<StoryProfile>* stories) {
  int cnt;
  scanf("%d", &cnt);
  for (int k = 0; k < cnt; k++) {
    char buf[10];
    sprintf(buf, "%d", k);  // NOLINT
    int num;
    scanf("%d", &num);
    StoryProfile story;
    story.story_id = string(buf);
    for (int i = 0; i < num; i++) {
      int word;
      scanf("%d", &word);
      story.keywords[word] = 1;
    }
    stories->push_back(story);
  }
}
void TestIndex() {
  using recomm_engine::indexing::InstantIndex;
  boost::shared_ptr<InstantIndex> index(InstantIndex::GetInstance());
  // Fake doc
  vector<StoryProfile> stories;
  // for (int i = 0; i < 5; i++) {
  //   StoryProfile s;
  //   char buf[10];
  //   buf[1] = '\0';
  //   buf[0] = '0' + i;
  //   // itoa(i, buf, 10);
  //   s.story_id = string(buf);
  //   s.keywords[i] = 1;
  //   s.keywords[i+5] = 1;
  //   stories.push_back(s);
  //   LOG(INFO) << "Creating story " << s.story_id;
  // }

  ParseInput(&stories);
  for (int i = 0; i < 5; i++) {
    StoryAddingRequest req;
    req.story = stories[i];
    sleep(3);
    LOG(INFO) << "Adding Story " << req.story.story_id;
    index->AddStory(req);
  }
  // sleep(80);
  while (true) {
    sleep(1);
    LOG(INFO) << "Printing index";
    // Print index
    for (int i = 0; i < 10; i++) {
     shared_ptr<PostingIterator> it = index->GetPostingList(i);
     if (!it.get()) {
       LOG(ERROR) << "No posting list for token " << i;
       continue;
     }
     cout << "Posting list for " << i << ":";
     while (it->HasValue()) {
       Posting p = it->GetValue();
       cout << p.local_docid << ",";
       it->Next();
     }
     cout << endl;
    }
  }
  sleep(60);
}
void Retrieval() {
  while (true) {
    Timer timer;
    timer.Begin();
    InstantIndex* index = InstantIndex::GetInstance();
    using boost::unordered_set;
    unordered_set<int64_t> global_id_set;
    unordered_set<LocalDocID> local_id_set;
    const int NUM_R_TOKEN = 5;
    vector<int64_t> tokens;
    for (int i = 0; i < NUM_R_TOKEN; i++) {
      tokens.push_back(rand() % NUM_TOKEN);  // NOLINT
    }
    map<int64_t, shared_ptr<PostingIterator> > iter_map =
       index->GetPostingListBatch(tokens);
    typedef map<int64_t, shared_ptr<PostingIterator> >::iterator Iter;
    for (Iter it = iter_map.begin(); it != iter_map.end(); ++it) {
      if (!it->second.get()) {
        continue;
      }
      shared_ptr<PostingIterator> pt = it->second;
      int cnt = 0;
      while (pt->HasValue()) {
        Posting p = pt->GetValue();
        local_id_set.insert(p.local_docid);
        pt->Next();
        cnt++;
      }
    }
    vector<LocalDocID> local_id_vec(local_id_set.begin(), local_id_set.end());
    map<LocalDocID, int64_t> global_id_map;
    index->TranslateToGlobalIDBatch(local_id_vec, &global_id_map);

    /*
    for (int i = 0; i < NUM_R_TOKEN; i++) {
      shared_ptr<PostingIterator> it = index->GetPostingList(
                                         rand() % NUM_TOKEN);  // NOLINT
      if (!it.get()) {
        continue;
      }
      int cnt = 0;
      while (it->HasValue()) {
        Posting p = it->GetValue();
        local_id_set.insert(p.local_docid);
        it->Next();
        cnt++;
      }
    }
    for (unordered_set<LocalDocID>::iterator it = local_id_set.begin();
         it != local_id_set.end(); ++it) {
      int64_t global_id;
      if (index->TranslateToGlobalID(*it, &global_id)) {
        global_id_set.insert(global_id);
      }
    }
    */
    retrieval_counter++;
    timer.End();
    latency = timer.GetMs();
    // if (timer.GetMs() > latency) {
    //   latency = timer.GetMs();
    // }
  }
}
void LoadTestIndexBuilding() {
  InstantIndex* index = InstantIndex::GetInstance();
  for (int i = 0; i < NUM_DOCS; i++) {
    StoryAddingRequest req;
    for (int k = 0; k < NUM_KEYWORDS; k++) {
      int word_id = rand() % NUM_TOKEN;  // NOLINT
      req.story.keywords[word_id] = 1;
    }
    req.story.__isset.keywords = true;
    usleep(5*1000);
    index->AddStory(req);
  }
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, false);
  InstantIndex* index = InstantIndex::GetInstance();
  // shared_ptr<boost::thread> build_thread(new boost::thread(
  //                           LoadTestIndexBuilding));
  vector<shared_ptr<boost::thread> > r_threads;
  for (int i = 0; i < 10; i++) {
    shared_ptr<boost::thread> r_thread(new boost::thread(Retrieval));
    r_threads.push_back(r_thread);
  }
  unsigned int last_counter = retrieval_counter;
  while (true) {
    sleep(1);
    LOG(INFO) << "Retrieval done:" << retrieval_counter - last_counter
    << " Latency: " << latency;
    last_counter = retrieval_counter;
    string info;
    index->GetState(&info);
    LOG(INFO) << info;
  }
  sleep(1000 * 1000);
  return 0;
}
