#ifndef _RECOMM_ENGINE_USER_PROFILE_STORAGE_H_
#define _RECOMM_ENGINE_USER_PROFILE_STORAGE_H_
#include "common.h"
#include "redis_cluster.h"
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>

namespace redis {
class Client;
}

namespace recomm_engine {
namespace idl {
class UserProfile;
}

class LocalDict;

class UserProfileStorage {
 public:
  UserProfileStorage();
  virtual ~UserProfileStorage();

  /**
   * return 0 - suces
   * return 1 - not exist
   * return 2 - storage failure
   */
  int GetUserProfile(const std::string& user_id,
                     idl::UserProfile* profile);

  /**
   * return 0 - suces
   * return 2 - storage failure
   */
  int PutUserProfile(const std::string& user_id,
                     idl::UserProfile& profile,
                     bool is_increment = false);
 protected:
  boost::scoped_ptr<LocalDict> user_profile_cache_;

  int life_in_seconds_;

  boost::scoped_ptr<redis_cluster::RedisCluster> user_profile_redis_;

  bool SetToRedis(redis::Client *redis, const std::string& user_id,
      idl::UserProfile& profile,
      int life_in_seconds = -1);

  void IncrementUserProfile(idl::UserProfile& org_proflie, 
                            idl::UserProfile& add_profile);

  DISALLOW_COPY_AND_ASSIGN(UserProfileStorage);
};

}  // namespace recomm_engine

#endif  // _RECOMM_ENGINE_USER_PROFILE_STORAGE_H_
