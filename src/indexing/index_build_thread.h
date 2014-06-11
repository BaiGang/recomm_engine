// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing  Bai)
// Description: Index builder

#ifndef SRC_INDEXING_INDEX_BUILD_THREAD_H_
#define SRC_INDEXING_INDEX_BUILD_THREAD_H_

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include "./base.h"
#include "./posting.h"
#include "gflags/gflags.h"
#include "./recomm_engine_types.h"
#include "util/concurrency/thread.h"
#include "util/concurrency/mutex.h"
#include "util/containers/producer_consumer_queue.h"

DECLARE_int32(doc_pool_size);
namespace recomm_engine {
namespace indexing {

/*
 * This is a daemon thread for index building.
 * Doc to build is added into a pool, from which this 
 * thread fetch docs and build index.
*/
class IndexBuildThread : public util::Thread {
  public:
    // Constructor
    IndexBuildThread(boost::unordered_map<int64_t, PostingList>& index_,  // NOLINT
                     util::RwMutex& _index_mutex,
                     boost::unordered_map<LocalDocID,
                     idl::IndexingAttachment>& _att,
                     util::RwMutex& _att_mutex);
    // Destructor
    ~IndexBuildThread();
    // Add story to buffer, waiting to be build into index
    bool AddStory(const idl::StoryAddingRequest& req);
    // Thread body, fetch story from buffer and build index
    void Run();

  private:
    // Build Index for a story
    bool AddStoryInternal(const idl::StoryAddingRequest& req);

  private:
    // index data structure and corresponding rw mutex
    boost::unordered_map<int64_t, PostingList>& index_;
    util::RwMutex& index_mutex_;
    // attachment and corresponding rw mutex
    boost::unordered_map<LocalDocID, idl::IndexingAttachment>& att_;
    util::RwMutex& att_mutex_;
    // Local docid counter
    LocalDocID local_docid_counter_;
    // doc pool for building, for buffering the stories to be built
    boost::shared_ptr<util::ProducerConsumerQueue<
                      idl::StoryAddingRequest> > doc_pool_;
};
}  // namespace indexing
}  // namespace recomm_engine
#endif  // SRC_INDEXING_INDEX_BUILD_THREAD_H_
