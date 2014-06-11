// Copyright 2013 Sina Inc. All rights reserved.
// Author: sijia2@staff.sina.com.cn (Sijia Yu)
// Description: retrieval handler supplied to the other class

#ifndef SRC_RETRIEVAL_RETRIEVAL_HANDLER_H_
#define SRC_RETRIEVAL_RETRIEVAL_HANDLER_H_


#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "../indexing/instant_index.h"
#include "./RecommEngine.h"
#include "./recomm_engine_types.h"
#include "./recomm_engine_constants.h"
#include "./index_reader.h"
#include "./ranker.h"
#include "./result_set.h"

#define  MAX_DOC_RETURN  30
namespace recomm_engine {
namespace retrieving {

class RetrievalHandler {
  public:
    RetrievalHandler();
    ~RetrievalHandler();

    int open();
    int retrieve(const idl::RetrievalRequest& request,
                  idl::RetrievalResponse* response);

  protected:
    void makeReply(idl::RetrievalResponse* response,
                  int doc_num, result_set* doc_set);

  protected:
    Ranker *m_ranker;   // compute score for per doc
};
}  // namespace retrieving
}  // namespace recomm_engine
#endif  // SRC_RETRIEVAL_RETRIEVAL_HANDLER_H_
