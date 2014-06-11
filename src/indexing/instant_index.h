// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing  Bai)
// Description: Index builder

#ifndef SRC_INDEXING_INSTANT_INDEX_H_
#define SRC_INDEXING_INSTANT_INDEX_H_

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <map>
#include <string>
#include <vector>
#include "gflags/gflags.h"
#include "./recomm_engine_types.h"
#include "./iterator.h"
#include "./base.h"
#include "./clean_old_story_thread.h"
#include "./replay_log_writer_thread.h"
#include "./index_build_thread.h"

// DECLARE_string(replay_log_dir);

namespace recomm_engine {
namespace indexing {

class InstantIndex;

/*
 * Main indexer
 * Building index
 * Sample Usage:
 *   InstantIndex* index = InstantIndex::GetInstance();
 *   // Your Code goes here
*/
class InstantIndex {
  public:
    // Singleton Method
    static InstantIndex* GetInstance();
    // Constructor & Destructor
    ~InstantIndex();

    // Build doc interface
    bool AddStory(const idl::StoryAddingRequest& req);

    // ********* Interface for retrieving ************
    typedef std::map<int64_t,
                     boost::shared_ptr<PostingIterator> > IteratorMapping;
    // Get Iterator
    boost::shared_ptr<PostingIterator> GetPostingList(int64_t token_id);
    IteratorMapping GetPostingListBatch(const std::vector<int64_t>& tokens);

    // Get Global docid from localdocid
    bool TranslateToGlobalID(LocalDocID local, int64_t* res);
    // Get global id in batch mode, return false once a fail
    // detected. The result will be fill with all local->global
    // mappings found
    bool TranslateToGlobalIDBatch(const std::vector<LocalDocID>& locals,
                                  std::map<LocalDocID, int64_t>* res);
    // Get State info, for debugging purpose
    void GetState(std::string *info);

  private:
    // Constructor
    InstantIndex();

    // Initialization
    bool Init();

    // Read log file and rebuild docs in log file
    void Replay();

  private:
    // Singleton instance
    static InstantIndex* single_instance_;
    // Mutex protecting the initialization of single instance
    static util::Mutex single_instance_mutex_;

  private:
    // **************** Daemon threads *****************
    // clean old stories from index/attachment/replay log
    boost::shared_ptr<CleanOldStoryThread> clean_old_story_thread_;
    // index building thread
    boost::shared_ptr<IndexBuildThread> index_build_thread_;
    // write doc into replay log
    boost::shared_ptr<ReplayLogWriterThread> replay_log_writer_thread_;
    // do the replaying when initializing
    boost::shared_ptr<boost::thread> replay_thread_;

    // **************** Main index **********************
    boost::unordered_map<int64_t, PostingList> index_;
    util::RwMutex index_mutex_;

    // *************** Attachment ***********************
    boost::unordered_map<LocalDocID, idl::IndexingAttachment> att_;
    // protecting local id coutner and attachment mapping
    util::RwMutex att_mutex_;
};
}  // namespace indexing
}  // namespace recomm_engine
#endif  // SRC_INDEXING_INSTANT_INDEX_H_
