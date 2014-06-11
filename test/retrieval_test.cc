// Copyright 2013 Sina Inc. All rights reserved.
// Author: sijia2@staff.sina.com.cn (Sijia Yu)
// Description: rank per doc
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include "./gtest.h"
#include "./retrieval/retrieval_handler.h"
#include "./recomm_engine_types.h"
#include "./indexing/instant_index.h"
#include "./indexing/posting.h"
#include "./city.h"

namespace recomm_engine {

using recomm_engine::idl::StoryProfile;
using recomm_engine::idl::StoryAddingRequest;
using recomm_engine::idl::RetrievalRequestInfo;
using recomm_engine::idl::RetrievalRequest;
using recomm_engine::idl::RetrievalResponse;
using recomm_engine::idl::RetrievalResult;
using recomm_engine::idl::RetrievalResponseCode;
using recomm_engine::indexing::InstantIndex;
using recomm_engine::indexing::PostingIterator;
using recomm_engine::indexing::Posting;
using recomm_engine::retrieving::RetrievalHandler;
class RetrievalTest : public ::testing::Test {
 public:
  virtual void SetUp();
  virtual void TearDown();
 protected:
  /* retrieval interface */
  int getRetrievalRequest(FILE *fp, std::vector<RetrievalRequestInfo>& keywords);
  void printRequest(RetrievalRequest& request);
  void printResponse(RetrievalResponse* response);
  void getStandardRetrievalResponse(std::vector<RetrievalResponse>& standardResponse);
  bool compare(RetrievalResponse& test_response, RetrievalResponse& standard_response);

  /* index interface */
  void ParseIndexInput(std::vector<StoryProfile>& stories);
  void LoadIndexData();

 protected:
  boost::scoped_ptr<RetrievalHandler> retrieval_handler_; 

};


void RetrievalTest::getStandardRetrievalResponse(std::vector<RetrievalResponse>& standardResponse) {
    const char* filename = "./test/retrieval_test_responses.bootstrap";
    FILE *fp = NULL;
    fp = fopen(filename, "r");

    if (fp == NULL) {
        LOG(ERROR) << "open file " << filename << " failed";
    }

    while (!feof(fp)) {
        RetrievalResponse temp_response;
        int status = 0, num_docs = 0;
        fscanf(fp, "%d%d", &status, &num_docs);
        temp_response.resp_code = (RetrievalResponseCode)(status);
        temp_response.num_results = num_docs;
        for (int j = 0; j < num_docs; j++){
            int64_t story_id = 0;
            double score = 0.0;
            fscanf(fp, "%ld:%lf", &story_id, &score);

            RetrievalResult temp_result;
            temp_result.story_id = story_id;
            temp_result.score = score;
            temp_response.results.push_back(temp_result);
         }

       standardResponse.push_back(temp_response);
    }

    fclose(fp);
}

bool RetrievalTest::compare(RetrievalResponse& test_response, RetrievalResponse& standard_response){
  if( (test_response.resp_code != standard_response.resp_code) ||
                (test_response.num_results != standard_response.num_results) )
     return false;

  for (int i = 0; i < test_response.num_results; i++) {
     int64_t story_id = 0;
     char buf[100];
     sprintf(buf, "%d", standard_response.results[i].story_id-1);
     story_id = CityHash64(buf, strlen(buf));
     if (test_response.results[i].story_id != story_id ||
                test_response.results[i].score != standard_response.results[i].score)
           return false;
  }

  return true;
}
void RetrievalTest::ParseIndexInput(std::vector<StoryProfile>& stories) {
  int cnt;
  const char* filename = "./test/index_test.bootstrap";
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    LOG(ERROR) << "open file failed";
    return;
  }
  fscanf(fp, "%d", &cnt);
  for (int k = 0; k < cnt; k++) {
    char buf[10];
    sprintf(buf, "%d", k);
    int num;
    fscanf(fp, "%d", &num);
    StoryProfile story;
    story.story_id = std::string(buf);
    for (int i = 0; i < num; i++) {
      int word;
     fscanf(fp, "%d", &word);
     story.keywords[word] = 1;
    }
    stories.push_back(story);
  }
  fclose(fp);
}

void  RetrievalTest::LoadIndexData() {
  std::vector<StoryProfile> stories;
  ParseIndexInput(stories);
  for (int i = 0; i < stories.size(); i++) {
    StoryAddingRequest req;
    req.story = stories[i];
    sleep(5);
    DLOG(INFO) << "Index:: Adding Story " << req.story.story_id;
       InstantIndex::GetInstance()->AddStory(req);
  }
  sleep(5);

  LOG(INFO) << "Printing index";
  // Print index
  for (int i = 0; i < 10; i++) {
  boost::shared_ptr<PostingIterator> it = InstantIndex::GetInstance()->GetPostingList(i);
  if (!it.get()) {
       LOG(ERROR) << "No posting list for token " << i;
       continue;
  }
  std::cout << "Posting list for " << i << ":";
  while (it->HasValue()) {
      Posting p = it->GetValue();
      std::cout << p.local_docid << ",";
      it->Next();
  }
  std::cout << std::endl;
  }
}


int RetrievalTest::getRetrievalRequest(FILE *fp,
                        std::vector<RetrievalRequestInfo>& keywords) {
  keywords.clear();
  if (feof(fp)) {
    return 0;
  }

  int num_keywords = 0;
  fscanf(fp, "%d", &num_keywords);
  for (int i = 0; i < num_keywords; i++) {
    int64_t keyword_id;
    double  weight = 0.0;
    fscanf(fp, "%ld:%lf", &keyword_id, &weight);
    RetrievalRequestInfo info;
    info.keyword = keyword_id;
    info.weight = weight;
    keywords.push_back(info);
  }
  return 1;
}

void RetrievalTest::printRequest(RetrievalRequest& request) {
  LOG(INFO) << "======================================================";
  LOG(INFO) << "the request info is:\n";
  for (int i = 0; i < request.keywords.size(); i++) {
      LOG(INFO) << i << " : " << request.keywords[i].keyword
                << ", " << request.keywords[i].weight;
  }
}

void RetrievalTest::printResponse(RetrievalResponse* response) {
  LOG(INFO) << "the response info is:\n";
  LOG(INFO) << "status = " << response->resp_code;
  LOG(INFO) << "num_docs = " << response->num_results;
  LOG(INFO) << "the doc list is:\n";
  for (int i = 0; i < response->results.size(); i++) {
      LOG(INFO) << i << " : " << response->results[i].story_id
                << ", " << response->results[i].score;
  }
  LOG(INFO) << "===========================================================";
}

void RetrievalTest::SetUp() {
  retrieval_handler_.reset(new RetrievalHandler());
  ASSERT_TRUE(retrieval_handler_ != NULL);
}

void RetrievalTest::TearDown() {
  retrieval_handler_.reset();
}
/*
TEST_F(RetrievalTest, PutGetData) {

  
}
*/
TEST_F(RetrievalTest, BootstrapData) {
  const char *filename = "./test/retrieval_test_requests.bootstrap";
  LOG(INFO) << "Load Data starting";
  LoadIndexData();
  std::vector<RetrievalResponse> standard_responses;
  getStandardRetrievalResponse(standard_responses);
  DLOG(INFO) << "retrieving starting..............";
  if (retrieval_handler_->open()) {
     LOG(ERROR) << "retrieval:: retrieval handler open failed!\n";
  } else {
       FILE* fp = fopen(filename, "r");
       if (fp == NULL) {
            LOG(ERROR) << "retrieval:: open file : "
            << filename << " failed.\n";
       } else {
    std::vector<RetrievalRequestInfo> keywords;
    int id = 0;
    while (getRetrievalRequest(fp, keywords)) {
        RetrievalRequest request;
        request.keywords = keywords;
        RetrievalResponse* response = new RetrievalResponse();
        // printRequest(request);
        retrieval_handler_->retrieve(request, response);
        printResponse(response);
        EXPECT_TRUE(compare(*response, standard_responses[id]));
        id++;
        delete response;
    }

  fclose(fp);
}
}
}
}  // namespace recomm_engine


