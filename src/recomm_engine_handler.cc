#include <algorithm>
#include <queue>
#include <deque>
#include <list>
#include <boost/thread/thread.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/format.hpp>
#include <boost/unordered_set.hpp>
#include "RecommEngine.h"
#include "recomm_engine_handler.h"
#include "story_profile_storage.h"
#include "user_profile_storage.h"
#include "algo_plugin_manager.h"
#include "retrieval/retrieval_handler.h"
#include "ialgo_plugin.h"

namespace recomm_engine {

RecommEngineHandler::RecommEngineHandler()
  : user_profile_storage_(new UserProfileStorage())
  , retrieval_handler_(new retrieving::RetrievalHandler()) {
  // Your initialization goes here
 
}

RecommEngineHandler::~RecommEngineHandler() {
  // destructions go here

}


// @brief Handler recommendation request.
void RecommEngineHandler::query(idl::RecommendationResponse& _return, const idl::RecommendationRequest& request) {
  // Your implementation goes here

  // Initialize the response as invalid.
  _return.response_code = idl::RRC_ERROR;

  // 1. FETCH USER PROFILE DATA ACCORDINGLY
  idl::UserProfile user_profile;
  int ret = user_profile_storage_->GetUserProfile(request.uid, 
                                             &user_profile);
  VLOG(10) << "Fetching user profile for [" << request.uid << "], ret [" << ret << "]";
  // 2. CHECK USER PROFILE AVAILABILITY
  //   TRUE if UserProfile exists AND UserProfile.keywords exists
  // 3.1  if ups not available, get the hottest and most local docs
  // FIXME(baigang): Use a specified method to check if the user profile is valid.
  if (ret != 0 || user_profile.keywords.empty() ) {
    // case when user profile is not available
    //FIXME(baigang):Get hot results.
    LOG(WARNING) << "User profile does not exist. uid=" << request.uid;
    _return.response_code = idl::RRC_UID_NOT_FOUND;
    return;
  }
 
  VLOG(30) << "UID is: <" << user_profile.uid << ">";
  VLOG(30) << "User keywords are: ";
  for (std::map<int64_t, int32_t>::const_iterator it = user_profile.keywords.begin();
       it != user_profile.keywords.end(); ++it) {
    VLOG(30) << "Keyword <" << it->first << ">, weight <" << it->second << ">";
  }
  VLOG(30) << "User history list:";
  for (std::vector<int64_t>::const_iterator it = user_profile.history.begin();
       it != user_profile.history.end(); ++it) {
    VLOG(30) << "Article hash code <" << *it << ">"; 
  }
 

  // 3.2  if available then iterate over docs and calculate proximity scores
  // TODO(sijia,baigang): Use the map structure rather than copy each item...
  idl::RetrievalRequest retrieval_request;
  for (std::map<int64_t, int32_t>::const_iterator it = 
          user_profile.keywords.begin();
       it != user_profile.keywords.end(); ++it) {
    idl::RetrievalRequestInfo info;
    info.keyword = it->first;
    info.weight = it->second;
    retrieval_request.keywords.push_back(info);
  }
  idl::RetrievalResponse retrieval_response;
  retrieval_handler_->retrieve(retrieval_request, &retrieval_response);

  if (retrieval_response.resp_code != idl::STATE_OK) {
    LOG(WARNING) << "Retrieval failed. uid=" << request.uid;
    //TODO(baigang): return hot/local news.
    _return.response_code = idl::RRC_NO_DOC;
    return;
  }
  
  boost::unordered_set<int64_t> existence;
  std::vector<idl::StoryProfile> candidates;
  for(std::vector<idl::RetrievalResult>::const_iterator iter =
      retrieval_response.results.begin();
      iter != retrieval_response.results.end(); ++iter) {
    if (user_profile.history.end() != std::find(user_profile.history.begin(),
                                                user_profile.history.end(),
                                                iter->story_id)) {
      VLOG(20) << "Skip story <" << iter->story_id << "> due to existence in history.";
      continue;
    }
    idl::StoryProfile story_profile;
    if (!StoryProfileStorage::GetInstance()->GetProfile(iter->story_id, &story_profile)) {
      LOG(WARNING) << "Failed getting profile from SPS.";
      continue;
    }

    if (existence.end() != existence.find(story_profile.signature)) {
      VLOG(20) << "Skip Story <" << iter->story_id << "> due to duplication.";
      continue;
    }
    existence.insert(story_profile.signature);
    candidates.push_back(story_profile);
  }
  if (candidates.size() == 0) {
    LOG(WARNING) << "No candidate for ranking. uid=" << request.uid;
    _return.response_code = idl::RRC_NO_CANDIDATE;
    return;
  }
  // 4. Do ranking, using the algo plugin
  boost::shared_ptr<IAlgoPlugin> ranking_plugin = AlgoPluginManager::GetInstance()->Select(user_profile.uid);
  int num_results = ranking_plugin->Rank(user_profile,
                                         candidates,
                                         &_return.results,
                                         false);

  if (_return.results.size() != static_cast<std::size_t>(num_results)) {
    // TODO(baigang): ranking failed. use default result
    LOG(WARNING) << "Ranking failed. Ret num is not equal to size of results. uid=" << request.uid;
    _return.response_code = idl::RRC_ERROR;
    return;
  } else if (_return.results.size() == 0) {
    // TODO(baigang): no ranked result, use default results
    LOG(WARNING) << "Empty ranked result. uid=" << request.uid;
    _return.response_code = idl::RRC_EMPTY_RANK_RESULT;
    return;
  }

  VLOG(20) << "Previously " << _return.results.size();
  VLOG(20) << "Requested " << request.topN;

  // 5. Truncate and filter
  std::string result_list;
  char score_buffer[128];
  std::size_t num_final_results = std::min(static_cast<std::size_t>(request.topN), _return.results.size());
  _return.results.resize(num_final_results);
  VLOG(20) << "Num of ranked results: [" << num_results << "], num of truncated results: [" << _return.results.size() << "]";
  for (std::size_t i = 0; i < num_final_results; ++i) {
    snprintf(score_buffer, 128, "%lf", _return.results[i].score);
    result_list += _return.results[i].story_id + ':' + score_buffer + ',';
  }
  _return.response_code = idl::RRC_OK;

  // LOGGING
  LOG(INFO) << "[RESULT][" << request.uid << "]["
            << _return.results.size() << "]["
            << result_list <<"]";

  return;
}

}  // namespace recomm_engine

