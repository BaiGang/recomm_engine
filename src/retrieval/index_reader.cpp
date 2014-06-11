// Copyright 2013 Sina Inc. All rights reserved.
// Author: sijia2@staff.sina.com.cn (Sijia Yu)
// Description: index reader

#include "./index_reader.h"
#include <map>
#include <vector>
#include "glog/logging.h"
using recomm_engine::indexing::InstantIndex;
namespace recomm_engine {
namespace retrieving {
IndexReader::IndexReader() {
}

IndexReader::~IndexReader() {
}

int IndexReader::open() {
    return 0;
}

int IndexReader::close() {
    endIntersect();
    return 0;
}

int IndexReader::beginIntersect(const idl::RetrievalRequest& request) {
    m_keyword_info = request.keywords;
    m_doc_hits_info.clear();

    // get the doc list of all keywords, and store into map
    for (std::size_t i = 0; i < m_keyword_info.size(); i++) {
      boost::shared_ptr<indexing::PostingIterator> iter =
InstantIndex::GetInstance()->GetPostingList(m_keyword_info[i].keyword);
      if (!iter.get()) {
          VLOG(10) << "keyword_id = "<< m_keyword_info[i].keyword
                     << ": No doclist ";
          continue;
      }
      VLOG(20) << "keyword_id = " << m_keyword_info[i].keyword
              << ": Doclist is :";
      while (iter->HasValue()) {
          // get the doc id and the keyword weight
          indexing::Posting onedoc = iter->GetValue();
          KeyOfDoc key_info;
          key_info.keyword_id = m_keyword_info[i].keyword;
          key_info.weight_in_doc = onedoc.weight;
          addHitKeywordToDocInfo(key_info, onedoc.local_docid);
          iter->Next();
          VLOG(4) << onedoc.local_docid << " ";
      }  // per doc in doclist
    }
    VLOG(20) << "doc_size=" << m_doc_hits_info.size();
    m_iter = m_doc_hits_info.begin();
  // dummy return value
  return 0;
}

void IndexReader::addHitKeywordToDocInfo(const KeyOfDoc& key_info,
                                        uint32_t doc_id) {
    std::map< uint32_t, std::vector<KeyOfDoc> >::iterator iter;
    iter = m_doc_hits_info.find(doc_id);
    if (iter != m_doc_hits_info.end()) {   // find the doc
       iter->second.push_back(key_info);
    } else {  // not find the doc
       std::vector<KeyOfDoc> keywords;
       keywords.push_back(key_info);
       m_doc_hits_info.insert(
       std::map< uint32_t, std::vector<KeyOfDoc> >::value_type
       (doc_id, keywords));
    }
}

int64_t IndexReader::getGlobalIDFromLocalID(const uint32_t& local_id) {
    int64_t g_id = local_id;
    if (InstantIndex::GetInstance()->
        TranslateToGlobalID(local_id, &g_id) == false) {
        LOG(ERROR) << "[retrieval_handler] "
        << local_id << " transfered to global id failed.\n";
    }
    VLOG(20) << "local_id = " << local_id << ",  global_id = " << g_id;
        return g_id;
}

int IndexReader::nextDoc(DocIterator* doc) {
    if (m_iter != m_doc_hits_info.end()) {
        int64_t g_id = getGlobalIDFromLocalID(m_iter->first);
        doc->story_id = g_id;
        doc->key_info = m_iter->second;
        m_iter++;
        return 1;
    } else {
        return 0;
    }
}

int IndexReader::endIntersect() {
    m_doc_hits_info.clear();
    m_keyword_info.clear();
    return 0;
}
}  // namespace retrieving
}  // namespace recomm_engine
