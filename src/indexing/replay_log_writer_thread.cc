// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing  Bai)
// Description: Index builder

#include <fstream>  // NOLINT
#include "./replay_log_writer_thread.h"
#include "util/base/thrift_util.h"
#include "util/base/timer.h"
#include "util/string/str_util.h"

using std::string;
using boost::shared_ptr;
using recomm_engine::idl::StoryAddingRequest;
using util::Timer;
using util::ThriftToString;
using util::ThriftToDebugString;
using util::ProducerConsumerQueue;

DEFINE_string(replay_log_dir, "./replay/", "dir for replay logs");
DEFINE_int32(replay_log_buffer, 1000, "buffer for writing replay log");

namespace recomm_engine {
namespace indexing {
// Constructor
ReplayLogWriterThread::ReplayLogWriterThread(
  int _doc_life_time,
  int _clean_interval) : doc_life_time_(_doc_life_time),
                         clean_doc_interval_(_clean_interval) {
  // Init pool
  pool_ = shared_ptr<ProducerConsumerQueue<StoryAddingRequest> >(
    new ProducerConsumerQueue<StoryAddingRequest>(FLAGS_replay_log_buffer, 0));
  CHECK(pool_) << "Fail to create doc pool for log writing";
  time_begin_ = Timer::CurrentTimeInS();
  time_end_ = time_begin_ + clean_doc_interval_;
}
// Thread body
void ReplayLogWriterThread::Run() {
  while (true) {
    StoryAddingRequest req;
    // fetch a request from buffer and write to log
    pool_->Get(&req);
    if (!Write(req)) {
      LOG(ERROR) << "Fail to write log";
    }
  }
}
// Do the writting
bool ReplayLogWriterThread::Write(const StoryAddingRequest& req) {
  unsigned int now = Timer::CurrentTimeInS();
  string file_name;
  // Update start and end if now time exceeds end
  if (now > time_end_) {
    time_begin_ = now;
    time_end_ = time_begin_ + clean_doc_interval_;
  }
  // construct
  string time_start_str = Timer::LocalTimeStrHMS(time_begin_);
  string time_end_str = Timer::LocalTimeStrHMS(time_end_);
  util::StringAppendF(&file_name, "%s/%s_%s_replay.log",
        FLAGS_replay_log_dir.c_str(),
        time_start_str.c_str(),
        time_end_str.c_str());
  // Append to file
  // Format: TIME SIZE THRIFT_CHUNK
  try {
    std::ofstream out(file_name.c_str(), std::ios::app);
    std::string serialized = util::ThriftToString(&req);
    size_t obj_size = serialized.size();
    out.write(reinterpret_cast<const char*>(&now), sizeof(unsigned int));
    out.write(reinterpret_cast<const char*>(&obj_size), sizeof(size_t));
    out.write(serialized.c_str(), obj_size);
    VLOG(4) << "Request saved " << ThriftToDebugString(&req);
  } catch(...) {
    return false;
  }
  return true;
}
// External interface:w
bool ReplayLogWriterThread::BackupStory(const idl::StoryAddingRequest& req) {
  return pool_->TryPut(req);
}
}  // namespace indexing
}  // namespace recomm_engine
