// Copyright 2013 Sina Inc. All rights reserved.
// Author: sijia2@staff.sina.com.cn (Sijia Yu)
// Description: rank per doc

#ifndef SRC_RETRIEVAL_INDEX_READER_H_
#define SRC_RETRIEVAL_INDEX_READER_H_

#include <map>
#include <vector>
#include "./recomm_engine_types.h"
#include "./recomm_engine_constants.h"
#include "../indexing/instant_index.h"
#include "./types.h"

/*
   read the index
*/

namespace recomm_engine {
namespace indexing {
class PostingIterator;
}
namespace retrieving {

typedef DocInfo DocIterator;

class IndexReader {
 public:
  IndexReader();
  ~IndexReader();
  int open();
  int close();

 /*
    get the doclist per keyword , and store keyword hited into the map;
 */
  int beginIntersect(const idl::RetrievalRequest& request);
  int endIntersect();

 /*
   check, and return one doc to retrieval_handler,
 */
  int nextDoc(DocIterator* doc);


 protected:
 /*
  insert the <doc_id ,keyword_id> into the map;
 */
  void addHitKeywordToDocInfo(const KeyOfDoc& key_info, uint32_t doc_id);

  /* 
   local id to global id
  */
  int64_t getGlobalIDFromLocalID(const uint32_t& local_id);

 protected:
  std::vector<idl::RetrievalRequestInfo> m_keyword_info;
  std::map< uint32_t, std::vector<KeyOfDoc> > m_doc_hits_info;
  std::map< uint32_t, std::vector<KeyOfDoc> >::iterator m_iter;
};
}  // namespace retrieving
}  // namespace recomm_engine
#endif  // SRC_RETRIEVAL_INDEX_READER_H_
