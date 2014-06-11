// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing  Bai)
// Description: Index builder

#ifndef SRC_INDEXING_REPLAY_LOG_WRITER_THREAD_H_
#define SRC_INDEXING_REPLAY_LOG_WRITER_THREAD_H_

#include "./recomm_engine_types.h"
#include "util/containers/producer_consumer_queue.h"
#include "util/concurrency/thread.h"
#include "gflags/gflags.h"

DECLARE_string(replay_log_dir);

namespace recomm_engine {
namespace indexing {
/*
 * Write the build doc into replay log
 */
class ReplayLogWriterThread : public util::Thread {
  public:
    // Constructor
    ReplayLogWriterThread(int32_t _doc_lifetime, int _clean_doc_interval);
    // External interface to backup a story into replay log
    bool BackupStory(const idl::StoryAddingRequest& req);
    // Thread body
    void Run();
  private:
     // Write to file
     bool Write(const idl::StoryAddingRequest& req);
  private:
    // life time of a doc
    int doc_life_time_;
    // how frequently the thread works
    int clean_doc_interval_;
    // current time bucket
    unsigned int time_begin_;
    unsigned int time_end_;
    // doc pool to buffer the docs to write
    boost::shared_ptr<util::ProducerConsumerQueue<
                       idl::StoryAddingRequest> > pool_;
};
}  // namespace indexing
}  // namespace recomm_engine
#endif  // SRC_INDEXING_REPLAY_LOG_WRITER_THREAD_H_
