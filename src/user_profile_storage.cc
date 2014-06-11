#include "user_profile_storage.h"
#include "util/base/thrift_util.h"
#include "recomm_engine_types.h"
#include "local_dict.h"
#include "redispp.h"

DEFINE_string(user_profile_cache_file, "./uec.leveldb", "Path of user entity cache.");
DEFINE_int32(user_profile_cache_mem_size, 32768, "Memory size of the user entity cache.");
DEFINE_string(redis_cluster_config_path, "./test/redis_cluster_test.conf", "Path to the config file for redis cluster.");
DEFINE_int32(user_profile_life_in_seconds, 172000, "user profile expire time.");

namespace recomm_engine {

UserProfileStorage::UserProfileStorage() 
  : user_profile_cache_(new LocalDict(
        FLAGS_user_profile_cache_file,
        FLAGS_user_profile_cache_mem_size)),
  user_profile_redis_(new redis_cluster::RedisCluster()) {
    bool ret = false;
    ret = user_profile_redis_->Initialize(FLAGS_redis_cluster_config_path);
    if (!ret) {
      LOG(ERROR) << "Failed init user profile stotage"; 
      exit(1);
    }

    life_in_seconds_ = FLAGS_user_profile_life_in_seconds;
}

UserProfileStorage::~UserProfileStorage() {
}

int UserProfileStorage::GetUserProfile(
    const std::string& user_id,
    idl::UserProfile* profile) {

  redis::Client *redis = NULL;
  pthread_mutex_t *mutex = NULL;
  if (!user_profile_redis_->GetRedisNode(user_id, &redis, &mutex)) {
    LOG(ERROR) << "Failed get redis node from redis cluster"; 
    return 2;   // storage failure
  }

  CHECK(redis != NULL);
  CHECK(mutex != NULL);

  int ret = 0;
  std::string value;
  pthread_mutex_lock(mutex);
  ret = redis->Get(user_id.c_str(), user_id.size(), &value);
  if (-1 == ret) {
      LOG(WARNING) << "redis error because [" << redis->last_error() << "]";
      pthread_mutex_unlock(mutex);
      return 2;
  }
  pthread_mutex_unlock(mutex);

  if (0 == ret) {
    if (!util::StringToThrift(value, profile)) {
       LOG(ERROR) << "Failed deserialization. ";
       return 2;
    }
  } else {
    if (1 == ret) {
      return 1;   // user_id not exist in redis
    }
  }

  return 0;
}

/* 
 * idl UserProfile define
 * struct UserProfile {
 *   1: required string uid;
 *   2: optional map<i64, i32> keywords;
 *   3: optional list<i32> topics;
 * }
 *
 * you must specified is or not serialized field [keywords, topics],
 *  before call this function
 */

int UserProfileStorage::PutUserProfile(
    const std::string& user_id,
    idl::UserProfile& profile,
    bool is_increment) {

  redis::Client *redis;
  pthread_mutex_t *mutex;
  if (!user_profile_redis_->GetRedisNode(user_id, &redis, &mutex)) {
    LOG(ERROR) << "Failed get redis node from redis cluster"; 
    return 2;   // storage failure
  } 

  //TODO redis_clusrt mutlti threads
  // un-increment
  if (!is_increment) {
    if (SetToRedis(redis, user_id, profile, life_in_seconds_)) {
      return 0;
    }
  } else {  // increment
    int ret = 0;
    idl::UserProfile org_profile;
    ret = GetUserProfile(user_id, &org_profile);
    if (2 != ret) {
      if (0 == ret) {   // user_id exist in redis 
        IncrementUserProfile(org_profile,  profile);
      }
      if (SetToRedis(redis, user_id, org_profile, life_in_seconds_)) {
        return 0;
      } 
    }
  }

  return 2;
}

bool UserProfileStorage::SetToRedis(
    redis::Client *redis,
    const std::string& user_id,
    idl::UserProfile& profile,
    int life_in_seconds) {
  std::string serialized_value = util::ThriftToString(&profile);
  if (redis->SetEx(user_id.c_str(), user_id.size(),
        serialized_value.c_str(), serialized_value.size(),
        life_in_seconds)) {
    return true;
  } else {
    LOG(ERROR) << "Failed set data to redis [key=>" <<
      user_id << ", value=>" << serialized_value << "]"; 
    return false;
  }
}

// just increment keywords value
void UserProfileStorage::IncrementUserProfile(
    idl::UserProfile& org_proflie,
    idl::UserProfile& add_profile) {

  for (std::map<int64_t, int32_t>::const_iterator it =
      add_profile.keywords.begin();
      it != add_profile.keywords.end(); ++it) {
    if (org_proflie.keywords.count(it->first)) {
      org_proflie.keywords[it->first] += it->second; 
    } else {
      org_proflie.keywords[it->first] = it->second;
    }
  }
}


}  // namespace recomm_engine

