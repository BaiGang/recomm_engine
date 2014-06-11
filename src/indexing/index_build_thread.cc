// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Description: Index builder


#include <map>
#include "util/base/timer.h"
#include "./index_build_thread.h"
#include "city.h"  // NOLINT
#include "./monitor_manager.h"

DEFINE_int32(doc_pool_size, 100000, "doc pool to build");

using std::map;
using std::pair;
using std::vector;
using boost::shared_ptr;
using boost::unordered_map;
using recomm_engine::idl::StoryProfile;
using recomm_engine::idl::StoryAddingRequest;
using util::ReaderMutexLock;
using util::WriterMutexLock;
using recomm_engine::idl::IndexingAttachment;
using util::ListNode;
using util::ProducerConsumerQueue;
using util::Timer;

namespace recomm_engine {
namespace indexing {

IndexBuildThread::IndexBuildThread(
  boost::unordered_map<int64_t, PostingList>& _index,
  util::RwMutex& _index_mutex,
  boost::unordered_map<LocalDocID, idl::IndexingAttachment>& _att,
  util::RwMutex& _att_mutex):
                  index_(_index), index_mutex_(_index_mutex),
                  att_(_att), att_mutex_(_att_mutex), local_docid_counter_(1) {
  // Create doc pool
  doc_pool_ = shared_ptr<ProducerConsumerQueue<StoryAddingRequest> >(
         new ProducerConsumerQueue<StoryAddingRequest>(FLAGS_doc_pool_size, 0));
  CHECK(doc_pool_.get()) << "Fail to init doc pool with size "
                    << FLAGS_doc_pool_size;
}
void IndexBuildThread::Run() {
  while (true) {
    StoryAddingRequest req;
    VLOG(5) << "Fetching doc from doc pool...";
    doc_pool_->Get(&req);
    VLOG(5) << "Doc fetched with id " << req.story.story_id;
    this->AddStoryInternal(req);
    VLOG(5) << "Doc Added to index";
  }
}

IndexBuildThread:: ~IndexBuildThread() {
}

bool IndexBuildThread::AddStoryInternal(const StoryAddingRequest& req) {
  // *********** Step1: Add to attachment first *****
  IndexingAttachment att;
  const StoryProfile& story = req.story;
  att.global_docid = CityHash64(story.story_id.c_str(),
                           story.story_id.size());
  LocalDocID local_id = kIllegalLocalDocID;
  {
    WriterMutexLock lk(att_mutex_);
    local_id = local_docid_counter_++;
		MonitorManager* mm = MonitorManager::Instance();
		mm->num_indexed_stories_current_++;
		mm->num_indexed_stories_total_++;
    // add to mapping
    att_[local_id] = att;
  }
  // *********** Step 2: Adding posting list *****
  vector<int64_t> new_tokens;
  typedef unordered_map<int64_t, PostingList>::iterator Iter;
  // First pass to detect new tokens
  {
    ReaderMutexLock lk(index_mutex_);
    for (map<int64_t, int32_t>::const_iterator it = story.keywords.begin();
         it != story.keywords.end(); ++it) {
         Iter idx_it = index_.find(it->first);
         if (idx_it == index_.end()) {
           new_tokens.push_back(it->first);
         }
    }
  }
  // create posting list for new tokens
  {
    WriterMutexLock lk(index_mutex_);
    int size = new_tokens.size();
    for (int i = 0; i < size; i++) {
      index_[new_tokens[i]] = PostingList();
    }
  }
  VLOG(4) << "New token count VS total token count "
          << new_tokens.size() << " VS " << story.keywords.size();
  // Add to posting list
  {
    ReaderMutexLock lk(index_mutex_);
    for (map<int64_t, int32_t>::const_iterator it = story.keywords.begin();
      it != story.keywords.end(); ++it) {
      Iter idx_it = index_.find(it->first);
      CHECK(idx_it != index_.end()) << "Fail to find token in index:"
                                    << it->first;
      PostingList& plist = idx_it->second;
      // token found, add to posting list
      ListNode<Posting>* p = plist.AllocatePosting();
      p->data->local_docid = local_id;
      p->data->weight = it->second;
      p->data->timestamp = Timer::CurrentTimeInS();
      plist.AddPosting(p);
    }
  }
  VLOG(3) << "Story pushed with story_id=" << story.story_id;
  return true;
}
bool IndexBuildThread::AddStory(const StoryAddingRequest& req) {
  return doc_pool_->TryPut(req);
}
}  // namespace indexing
}  // namespace recomm_engine
