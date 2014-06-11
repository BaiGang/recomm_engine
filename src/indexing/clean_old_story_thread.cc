// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing  Bai)
// Description: The thread cleans old story docs

#include <boost/unordered_set.hpp>
#include <boost/filesystem.hpp>
#include <vector>
#include <string>
#include "util/containers/lockless_list.h"
#include "./clean_old_story_thread.h"
#include "util/base/timer.h"
#include "util/string/str_util.h"
#include "./monitor_manager.h"

using std::vector;
using std::string;
using boost::unordered_map;
using util::RwMutex;
using util::ReaderMutexLock;
using util::WriterMutexLock;
using util::Timer;
using util::ListNode;
using recomm_engine::idl::IndexingAttachment;
using boost::unordered_map;
using boost::unordered_set;
using util::SplitString;

DEFINE_int32(clean_doc_interval, 200,
  "interval in seconds to clean old doc");
DEFINE_int32(mark_and_clear_sleep_time, 10,
  "sleep time between marking finish and clear begin, in seconds");
DEFINE_int32(doc_life_time, 3660,
  "how long time the doc will reside in index, in secons");

namespace recomm_engine {
namespace indexing {

// Constructor
CleanOldStoryThread::CleanOldStoryThread(
  unordered_map<int64_t, PostingList>& _index,
  RwMutex& _index_mutex,
  unordered_map<LocalDocID, IndexingAttachment>& _att,
  util::RwMutex& _att_mutex, const std::string& _replay_log_dir) :
     index_(_index), index_mutex_(_index_mutex),
     att_(_att), att_mutex_(_att_mutex),
     should_run_(true), delete_doc_cnt_(0), replay_log_dir_(_replay_log_dir) {
}

// Destructor
CleanOldStoryThread::~CleanOldStoryThread() {
  should_run_ = false;
}

// Thread body
void CleanOldStoryThread::Run() {
  typedef unordered_map<int64_t, PostingList>::iterator Iter;
  while (should_run_) {
    sleep(FLAGS_clean_doc_interval);
    // Do the Marking
    {
      ReaderMutexLock lk(index_mutex_);
      // Generate target time for end mark
      int64_t target_time = Timer::CurrentTimeInS() - FLAGS_doc_life_time;
      for (Iter it = index_.begin(); it != index_.end(); ++it) {
        PostingList& plist = it->second;
        const ListNode<Posting>* cur = plist.GetFakeFront();
        CHECK(cur != NULL) << "Broken list";
        // Find the end mark
        while (cur->next != NULL && cur->next->data->timestamp >= target_time) {
          cur = cur->next;
        }
        // Set end mark
        plist.SetEndMark(cur);
        if (cur != plist.GetFakeFront()) {
          VLOG(5) << "Mark set in posting list for token " << it->first
                     << " at doc " << cur->data->local_docid;
        } else {
          VLOG(5) << "Mark set in posting list for token " << it->first
                     << " at the very first begining";
        }
      }
    }

    // sleep till clearing
    VLOG(3) << "mark done, sleep for " << FLAGS_mark_and_clear_sleep_time;
    sleep(FLAGS_mark_and_clear_sleep_time);

    // Do the Clearing
    unordered_set<LocalDocID> doc_to_delete;
    {
      // We still use read lock here because we never delete the token entries
      // in the index
      ReaderMutexLock lk(index_mutex_);
      for (Iter it = index_.begin(); it != index_.end(); ++it) {
        int cnt = 0;
        PostingList& plist = it->second;
        const ListNode<Posting>* mark = plist.GetEndMark();
        if (mark) {
          const ListNode<Posting> * cur = mark->next;
          // collect the docs to delete from attachment
          while (cur != NULL) {
            cnt++;
            doc_to_delete.insert(cur->data->local_docid);
            cur = cur->next;
          }
        }
        VLOG(5) << cnt << " docs deleted for token " << it->first;
        // Erase after mark
        plist.EraseAfter(mark);
        // Reset the end mark
        plist.SetEndMark(NULL);
      }
    }
    // delete attahment
    {
      WriterMutexLock lk(att_mutex_);
      for (unordered_set<LocalDocID>::iterator it = doc_to_delete.begin();
           it != doc_to_delete.end(); ++it) {
        unordered_map<LocalDocID, IndexingAttachment>:: iterator att_it =
          att_.find(*it);
        // Erase doc
        if (att_it != att_.end()) {
          att_.erase(att_it);
        } else {
          LOG(ERROR) << "Fail to erase doc with local id " << *it;
        }
      }
			MonitorManager* mm = MonitorManager::Instance();
			int cnt = doc_to_delete.size();
			mm->num_indexed_stories_total_ -= cnt;
    }
    // increase counter
    delete_doc_cnt_ += doc_to_delete.size();
    VLOG(3) << doc_to_delete.size() << " doc deleted and mark reset";
    VLOG(3) << "Total doc deleted " << delete_doc_cnt_;
    // Remove old replay logs
    CleanReplayLog();
  }
}
// Clear the old replay log
void CleanOldStoryThread::CleanReplayLog() {
  using namespace boost::filesystem;  // NOLINT
  path dir(replay_log_dir_);
  directory_iterator it(dir);
  directory_iterator end_it;
  vector<path> to_remove;
  // Time check point, log file before this check
  // point should be removed
  unsigned int check_point = Timer::CurrentTimeInS()
                             - FLAGS_doc_life_time;
  // Check point string, formatted as YYMMDDHHMMSS
  string cp_string = Timer::LocalTimeStrHMS(check_point);
  for (; it != end_it; ++it) {
    path p = it->path();
    string fname = p.native();
    vector<string> segs;
    SplitString(fname, '/', &segs);
    fname = segs[segs.size() - 1];
    segs.clear();
    SplitString(fname, '_', &segs);
    if (segs.size() < 2) {
      LOG(ERROR) << "Illegal file name " << fname;
      continue;
    }
    // Check end time of the log against check point
    if (cp_string > segs[1]) {
      to_remove.push_back(it->path());
    }
  }
  // Remove the files
  for (size_t i = 0; i < to_remove.size(); i++) {
    LOG(INFO) << "Removing " << to_remove[i].native();
    try {
      remove((to_remove[i]));
    } catch(...) {
      LOG(ERROR) << "Fail to remove " << to_remove[i].native();
    }
  }
}
}  // namespace indexing
}  // namespace recomm_engine
