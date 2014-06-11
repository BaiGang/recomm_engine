#!/usr/local/bin/thrift --gen cpp:pure_enums --gen java --gen perl

include "fb303.thrift"

/*
 * @author baigang
 * Defines recomm_engine interface. 
 */


namespace cpp recomm_engine.idl
namespace java com.sina.recomm.engine.idl
namespace perl Recomm


/*******************************************************************/
/**  Recommendation interfaces. */
/*******************************************************************/
struct RecommendationRequest {
  1: required string uid;
  2: required string ip;
  3: required i16 topN;
}

enum RecommendationResponseCode {
  RRC_OK = 0,
  RRC_ERROR,
  RRC_UID_NOT_FOUND,
  RRC_NO_DOC,
  RRC_NO_CANDIDATE,
  RRC_NO_PROFILE,
  RRC_EMPTY_RANK_RESULT
}

struct RecommendationResult {
  1: required string story_id;
  2: required double score = 100.0;
  3: optional string debug_info;
}

struct RecommendationResponse {
  1: required RecommendationResponseCode response_code = RecommendationResponseCode.RRC_OK;
  2: required list<RecommendationResult> results;
}

service RecommEngine extends fb303.FacebookService {
  RecommendationResponse query(1: RecommendationRequest request);
}

/*******************************************************************/
/**  Story management interfaces. */
/**  - a story is a candidate to be recommended */
/*******************************************************************/

enum StoryManagementStatus {
  SMS_OK = 0,
  SMS_ERROR = 1;
}

struct StoryProfile {
  1: required string story_id;
  2: required map<i64, i32> keywords;
  3: optional list<i32> topics;
  4: required i64 signature;  // used in dedup.
}

struct StoryAddingRequest {
  1: required StoryProfile story;
}

struct StoryAddingResponse {
  1: required StoryManagementStatus status = StoryManagementStatus.SMS_OK;
}

service StoryManagement {
  StoryAddingResponse add_story(1: StoryAddingRequest request);
}

/*******************************************************************/
/**  Other auxilary data structures.  */
/**  - The user profile */
/**  - The item profile */
/*******************************************************************/

struct UserProfile {
  1: required string uid;
  2: required map<i64, i32> keywords;
  3: optional list<i32> topics;
  4: required list<i64> history;
}

enum UserEngagementType {
  UET_CLICK = 0,
  UET_FAVORITE = 2,
  UET_SHARE = 3,
  UET_LIKE = 4;
}

struct UserHistoryItem {
  1: required string story_id;
  2: optional UserEngagementType type = UserEngagementType.UET_CLICK;
  3: optional i64 timestamp = 0;
}

struct UserHistory {
  1: list<UserHistoryItem> history_items;
}


/*******************************************************************/
/** query engine interfaces. */
/*******************************************************************/

struct IndexingAttachment {
  1:required i64 global_docid;
}

struct RetrievalRequestInfo {
  1: required i64  keyword;
  2: optional i32  weight = 0;
}

struct RetrievalRequest {
  1: required list<RetrievalRequestInfo> keywords;
  2: optional i16 debug_level = 0;
  3: optional i16 log_level = 0;
}

struct RetrievalResult{
  1: required i64    story_id;
  2: optional double score = 0.0;
}

enum RetrievalResponseCode {
	STATE_OK = 0,
	STATE_ERROR,
	STATE_KEYWORD_NOT_FOUND,
    
}

struct RetrievalResponse {
  1: required RetrievalResponseCode resp_code = RetrievalResponseCode.STATE_OK;
  2: optional i32 num_results = 0;
  3: optional list<RetrievalResult> results;
}

service RetrievalEngine {
  RetrievalResponse search(1: RetrievalRequest request);
}
