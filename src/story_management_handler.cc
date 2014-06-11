#include "story_management_handler.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>
#include "leveldb/slice.h"
#include "city.h"
#include "story_profile_storage.h"
#include "indexing/instant_index.h"


typedef long long i64;

namespace ptime = boost::posix_time;

namespace recomm_engine {

StoryManagementHandler::StoryManagementHandler() {
}

StoryManagementHandler::~StoryManagementHandler() {
}

void StoryManagementHandler::add_story(idl::StoryAddingResponse& _return,
                                  const idl::StoryAddingRequest& request) {

  indexing::InstantIndex* inverted_index = indexing::InstantIndex::GetInstance();
  StoryProfileStorage* forward_index = StoryProfileStorage::GetInstance();

  VLOG(20) << "Adding story [" << request.story.story_id << "], signature is [" << request.story.signature << "]";
  for (std::map<int64_t, int32_t>::const_iterator it = request.story.keywords.begin();
       it != request.story.keywords.end(); ++it) {
    VLOG(30) << "Token <" << it->first << ">, weight [" << it->second << "]";
  }
  
  if (!inverted_index->AddStory(request)) {
    LOG(ERROR) << "Failed adding story [" << request.story.story_id << "] into invert index.";
    _return.status = idl::SMS_ERROR;
    return;
  }
  int64_t story_hash = CityHash64(request.story.story_id.data(),
                                  request.story.story_id.length());
  VLOG(20) << "Story hash is [" << story_hash << "]";
  if (!forward_index->AddProfile(
      story_hash,
      request.story)) {
    LOG(ERROR) << "Failed adding story [" << request.story.story_id << "] into forward index.";
    _return.status = idl::SMS_ERROR;
  }

  _return.status = idl::SMS_OK;
}


}  // namespace recomm_engine



