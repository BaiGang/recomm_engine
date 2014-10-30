// Copyright 2013 Sina Inc. All rights reserved.
// Author: sijia2@staff.sina.com.cn (Sijia Yu)
// Description: retrieval handler supplied to the other class

#include "./retrieval_handler.h"

DEFINE_int32(max_doc_return, 100, "max doc num return to the recomm handler");
DEFINE_int32(hit_keywords_num, 2, "the lower bounds of the keywords hit");
DEFINE_double(hit_keywords_ratio, 0.2, "the ratio of the keywords hit");

namespace recomm_engine {

namespace retrieving {
RetrievalHandler::RetrievalHandler() {
  m_ranker = new Ranker();
}

RetrievalHandler::~RetrievalHandler() {
  m_ranker->close();

  if (m_ranker)
    delete m_ranker;
}


int RetrievalHandler::open() {
  return 0;
}

int RetrievalHandler::retrieve(const idl::RetrievalRequest& request,
                               idl::RetrievalResponse* response) {
  result_set doc_set;

  IndexReader index_reader;
  index_reader.open();
  index_reader.beginIntersect(request);
  DocIterator iter;

  VLOG(20) << "the result doc list :";
  int id = 0;

  while (index_reader.nextDoc(&iter)) {  // get_doc
    id++;
    double score = m_ranker->compute_rank(&iter);
    if (score < FLAGS_hit_keywords_ratio * request.keywords.size()) {
       VLOG(20) << "skip, doc_id = " << iter.story_id << ", hit_num = " << score;
       continue;
    }
    doc_set.insert_doc(iter.story_id, score);
    VLOG(20) << id << "th: doc_id=" << iter.story_id;
  }

  index_reader.endIntersect();

  int doc_num = 0;
  doc_num = doc_set.dump();
  double filter_kw_size = FLAGS_hit_keywords_ratio * request.keywords.size();
  LOG(INFO) << "kw_size = " << request.keywords.size() << ", filter_kw_size = " << filter_kw_size 
            << ", result: total_doc_num = " << doc_num << ", max_doc_return = " << FLAGS_max_doc_return;
  doc_num = (doc_num > FLAGS_max_doc_return) ? FLAGS_max_doc_return : doc_num;
  makeReply(response, doc_num, &doc_set);
  return 0;
}

void RetrievalHandler::makeReply(idl::RetrievalResponse* response,
                                 int doc_num, result_set* doc_set ) {
  if (doc_num == 0) {
      response->resp_code = idl::STATE_KEYWORD_NOT_FOUND;
  }
  response->resp_code = idl::STATE_OK;
  response->num_results = doc_num;

  for (int i = 0; i < doc_num; i++) {
      idl::RetrievalResult temp;
      temp.story_id = doc_set->doc(i).story_id;
      temp.score = doc_set->doc(i).score;
      response->results.push_back(temp);
  }
}

}  // namespace retrieving
}  // namespace recomm_engine
