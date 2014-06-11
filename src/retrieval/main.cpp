// Copyright 2013 Sina Inc. All rights reserved.
// Author: sijia2@staff.sina.com.cn (Sijia Yu)
// Description: a sample
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "./retrieval_handler.h"
#include "glog/logging.h"
#include "./testIndex.h"

using namespace recomm_engine;
int getRequest(FILE *fp, std::vector<idl::RetrievalRequestInfo>& keywords) {
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
      idl::RetrievalRequestInfo info;
      info.keyword = keyword_id;
      info.weight = weight;
      keywords.push_back(info);
  }
  return 1;
}

void printRequest(const idl::RetrievalRequest& request) {
  VLOG(30) << "======================================================";
  VLOG(30) << "the request info is: ";
  for (int i = 0; i < request.keywords.size(); i++) {
    VLOG(30) << i << " : " << request.keywords[i].keyword
              << ", " << request.keywords[i].weight;
  }
}

void printResponse(idl::RetrievalResponse& response) {
  VLOG(30) << "the response info is:";
  VLOG(30) << "status = " << response.resp_code;
  VLOG(30) << "num_docs = "<< response.num_results;
  VLOG(30) << "the doc list is:";
  for (int i = 0; i < response.results.size(); i++) {
    VLOG(30) << i << " : " << response.results[i].story_id
              << ", " << response.results[i].score;
  }
  LOG(INFO) << "===========================================================";
}
int main(int argc, char ** argv) {
  testIndex* test_index = new testIndex();
  if (test_index->open())
    return 0;
  sleep(30);

  using recomm_engine::retrieving::RetrievalHandler;
  if (argc != 2) {
     LOG(INFO) << "usage: input-file.";
     return -1;
  }

  char *filename = argv[1];
  RetrievalHandler* retrieval_handler = new RetrievalHandler();
  if (retrieval_handler->open()) {
    LOG(ERROR) << "retrieval handler open failed!";
  } else {
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
      LOG(ERROR) << "open file : " << filename << " failed.\n";
    } else {
      std::vector<idl::RetrievalRequestInfo> keywords;
      while (getRequest(fp, keywords)) {
          idl::RetrievalRequest request;
          request.keywords = keywords;
          idl::RetrievalResponse response;
          printRequest(request);
          retrieval_handler->retrieve(request, response);
          printResponse(response);
      }
      fclose(fp);
    }
  }
  delete retrieval_handler;
  test_index->close();
  return 0;
}
