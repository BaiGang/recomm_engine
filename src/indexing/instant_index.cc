// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing  Bai)
// Description: Index builder

#include <boost/filesystem.hpp>
#include <fstream>  // NOLINT
#include "./instant_index.h"
#include "util/concurrency/mutex.h"
#include "util/base/atomic.h"
#include "util/base/thrift_util.h"
#include "./recomm_engine_types.h"
#include "util/string/str_util.h"
#include "city.h"  // NOLINT

using std::vector;
using std::string;
using std::map;
using boost::shared_ptr;
using util::ReaderMutexLock;
using boost::unordered_map;
using util::WriterMutexLock;
using recomm_engine::idl::StoryAddingRequest;
using recomm_engine::idl::IndexingAttachment;
using util::ThriftToDebugString;
using util::StringToThrift;
using util::StringAppendF;

DEFINE_int32(replay_buffer_size, 1024 * 1024 * 1024, "single obj buffer size");
namespace recomm_engine {
namespace indexing {

InstantIndex* InstantIndex::single_instance_ = NULL;
util::Mutex InstantIndex::single_instance_mutex_;
// Constructor
InstantIndex::InstantIndex() {
}

// Destructor
InstantIndex::~InstantIndex() {
}

// Get singleton instantce with double check
InstantIndex* InstantIndex::GetInstance() {
  if (single_instance_ == NULL) {  // first check
    util::MutexLock lk(single_instance_mutex_);
    if (single_instance_ == NULL) {
      InstantIndex* tmp = new InstantIndex();
      CHECK(tmp->Init()) << "Fail to initialize ";
      single_instance_ = tmp;
      util::AtomicPointerAssgin(&single_instance_, tmp);
    }
    return single_instance_;
  } else {
    return single_instance_;
  }
}

bool InstantIndex::Init() {
  // create daemon threads
  // create index building thread
  index_build_thread_ = shared_ptr<IndexBuildThread>(
    new IndexBuildThread(index_, index_mutex_, att_, att_mutex_));
  CHECK(index_build_thread_.get()) << "Fail to create index_build_thread";
  // create trash cleaner
  clean_old_story_thread_ = shared_ptr<CleanOldStoryThread>(
    new CleanOldStoryThread(index_, index_mutex_, att_, att_mutex_,
                            FLAGS_replay_log_dir));
  CHECK(clean_old_story_thread_.get()) << "Fail to init clean_old_story_thread";

  // create log writter
  replay_log_writer_thread_ = shared_ptr<ReplayLogWriterThread>(
    new ReplayLogWriterThread(FLAGS_doc_life_time, FLAGS_clean_doc_interval));
  CHECK(replay_log_writer_thread_.get())
        << "Fail to create replay_log_writer_thread";
  // Start index build thread
  index_build_thread_->Start();
  // Start the replay process
  replay_thread_.reset(new boost::thread(&InstantIndex::Replay, this));
  return true;
}

// Build a story into index
bool InstantIndex::AddStory(const StoryAddingRequest& req) {
  // Send the story to index building thread to build index
  bool ret = index_build_thread_->AddStory(req);
  if (!ret) {
    LOG(ERROR) << "Fail to put doc into build buffer";
    LOG(ERROR) << "May be we should make buffer bigger enough";
  }
  // Send the story to replay log writter to write log
  if (!replay_log_writer_thread_->BackupStory(req)) {
    LOG(INFO) << "Fail to backup story " << req.story.story_id;
  }
  return ret;
}

// Get Posting list by a token
shared_ptr<PostingIterator> InstantIndex::GetPostingList(
        int64_t token_id) {
  ReaderMutexLock lk(index_mutex_);
  unordered_map<int64_t, PostingList>::iterator it =
    index_.find(token_id);
  if (it != index_.end()) {
    shared_ptr<PostingIterator> ret(new PostingIteratorImpl(it->second));
    return  ret;
  } else {
    shared_ptr<PostingIterator> ret;
    return ret;
  }
}

// Get posting list by token in batch mdde
typedef std::map<int64_t, boost::shared_ptr<PostingIterator> > IteratorMapping;
IteratorMapping InstantIndex::GetPostingListBatch(
  const vector<int64_t>& tokens) {
  ReaderMutexLock lock(index_mutex_);
  int size = tokens.size();
  map<int64_t, shared_ptr<PostingIterator> > ret;
  for (int i = 0; i < size; i++) {
    unordered_map<int64_t, PostingList>::iterator it =
      index_.find(tokens[i]);
    if (it == index_.end()) {
      continue;
    } else {
      shared_ptr<PostingIterator> tmp(new PostingIteratorImpl(it->second));
      ret[tokens[i]] = tmp;
    }
  }
  return ret;
}

// Translate localdocid to global docid
bool InstantIndex::TranslateToGlobalID(LocalDocID  local_id,
        int64_t* res) {
  ReaderMutexLock lk(att_mutex_);
  typedef unordered_map<LocalDocID, IndexingAttachment>::iterator Iter;
  Iter it = att_.find(local_id);
  if (it != att_.end()) {
    VLOG(6) << "Global dociid found:" << local_id << "->"
            << it->second.global_docid;
    *res = it->second.global_docid;
    return true;
  } else {
    LOG(ERROR) << "Fail to find global id for " << local_id
               << ", may be erased";

    return false;
  }
}

// Translate in batch mode
bool InstantIndex::TranslateToGlobalIDBatch(const vector<LocalDocID>& locals,
                                            map<LocalDocID, int64_t>* res) {
  ReaderMutexLock lk(att_mutex_);
  bool ret = true;
  typedef unordered_map<LocalDocID, IndexingAttachment>::iterator Iter;
  int size =  locals. size();
  for (int i = 0; i < size; i++) {
    Iter it = att_.find(locals[i]);
    if (it != att_.end()) {
      (*res)[locals[i]] = att_[locals[i]].global_docid;
    } else {}
  }
  ret = locals.size() == res->size();
  if (!ret) {
    LOG(ERROR) << locals.size() - res->size() << " mappings not found";
  }
  return ret;
}

// Do the replay
void InstantIndex::Replay() {
  using namespace boost::filesystem;  // NOLINT
  path dir(FLAGS_replay_log_dir);
  directory_iterator it(dir);
  directory_iterator end_it;
  // Iterator all files in director
  for (; it != end_it; ++it) {
    string fname = it->path().native();
    unsigned int timestamp;
    size_t obj_size;
    StoryAddingRequest req;
		int cnt = 0;
    try {
      std::ifstream in(fname.c_str());
      // Read
      while (!in.eof()) {
        // read timestamp
        // TODO(yanbing3): should we ignore old doc?
        in.read(reinterpret_cast<char*>(&timestamp),
                sizeof(unsigned int));
        VLOG(10) << "read timestamp " << timestamp;
        // read object size
        in.read(reinterpret_cast<char*>(&obj_size),
                sizeof(size_t));
        VLOG(10) << "read obj size " << obj_size;
        // read object
        char buf[obj_size + 1024];  // NOLINT
        in.read(buf, obj_size);
        string str(buf, obj_size);
        VLOG(10) << "string size:" << str.size();
        // Deserialize
        if (!StringToThrift(str, &req)) {
          LOG(ERROR) << "Fail to read obj from " << fname;
          continue;
        }
        VLOG(10) << "Dump requst :" << ThriftToDebugString(&req);
        // Add to build buffer
        VLOG(10) << "Restoring doc " << req.story.story_id;
        index_build_thread_->AddStory(req);
				cnt++;
      }
    } catch(...) {
      LOG(ERROR) << "Error reading replay log " << fname;
    }
		LOG(INFO) << cnt << " stories read from " << fname;
  }
  // Start clean thread and replay log writer thread after

  // NOTE: We start the thread after replay process preventing
  // doing the replay on newly created log file
  clean_old_story_thread_->Start();
  replay_log_writer_thread_->Start();
}
void InstantIndex::GetState(string *info) {
  ReaderMutexLock lk(index_mutex_);
  ReaderMutexLock lka(att_mutex_);
  StringAppendF(info, "Total Token num: %d ", index_.size());
  StringAppendF(info, "Total docs: %d\n", att_.size());
  // StringAppendF(info, "Posting list info:");
  // typedef unordered_map<int64_t, PostingList>::iterator Iter;
  // for (Iter it = index_.begin(); it != index_.end(); ++it) {
  //   StringAppendF(info, "  %ld:%u\n", it->first, it->second.Size());
  // }
}
}  // namespace indexing
}  // namespace recomm_engine
