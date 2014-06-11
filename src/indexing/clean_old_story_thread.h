// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing  Bai)
// Description: The thread cleans old story docs

#ifndef SRC_INDEXING_CLEAN_OLD_STORY_THREAD_H_
#define SRC_INDEXING_CLEAN_OLD_STORY_THREAD_H_


#include <boost/unordered_map.hpp>
#include <string>
#include "./posting.h"
#include "./base.h"
#include "gflags/gflags.h"
#include "util/concurrency/thread.h"
#include "util/concurrency/mutex.h"
#include "./recomm_engine_types.h"

DECLARE_int32(clean_doc_interval);
DECLARE_int32(doc_life_time);

namespace recomm_engine {
namespace indexing {

/*
 * Daemon thread cleaning old docs from index
*/
class CleanOldStoryThread : public util::Thread {
  public:
    // Constructor
    CleanOldStoryThread(
      boost::unordered_map<int64_t, PostingList>& _index,
      util::RwMutex& index_mutex_,
      boost::unordered_map<LocalDocID, idl::IndexingAttachment>& _att,
      util::RwMutex& _att_mutex,
      const std::string& _replay_log_dir);
    // Destructor
    ~CleanOldStoryThread();
    // thread body
    void Run();

  private:
    // Remove old replay logs
    void CleanReplayLog();

  private:
    // index related
    boost::unordered_map<int64_t, PostingList>& index_;
    util::RwMutex& index_mutex_;

    // attachmente related
    boost::unordered_map<LocalDocID, idl::IndexingAttachment>& att_;
    util::RwMutex& att_mutex_;
    // should run
    bool should_run_;

    // delete doc counter
    int64_t delete_doc_cnt_;
    // replay log dir
    std::string replay_log_dir_;
};

}  // namespace indexing
}  // namespace recomm_engine

#endif  // SRC_INDEXING_CLEAN_OLD_STORY_THREAD_H_
