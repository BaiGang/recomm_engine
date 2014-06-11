#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include "gtest.h"
#include "user_profile_storage.h"
#include "recomm_engine_types.h"
#include "city.h"
#include "common.h"

#define USER_NUM 5
#define KEYWORD_NUM 10
#define TOPIC_NUM 160

namespace recomm_engine {

class UserProfileStorageTest : public ::testing::Test {
 public:
  virtual void SetUp();
  virtual void TearDown();
 protected:
  UserProfileStorage *user_profile_storage_;

};

void UserProfileStorageTest::SetUp() {
  user_profile_storage_ = new UserProfileStorage();
}

void UserProfileStorageTest::TearDown() {
  delete user_profile_storage_;
}

TEST_F(UserProfileStorageTest, PutUserProfile) {
  LOG(INFO) << "start put user profile";
  for (int i = 0; i < USER_NUM; ++i) {
      idl::UserProfile user_profile;
      char tmp[10];
      sprintf(tmp, "%d", i);
      user_profile.uid = "user_" + std::string(tmp);
      for (int64_t j = 0; j < KEYWORD_NUM; ++j) {
        user_profile.keywords[j] = rand() % 10000; 
        LOG(INFO) << "PutUserProfile " << user_profile.uid <<" : keyword ---->" \
                  << "[" << j << "," << user_profile.keywords[j] << "]";
      }
      for (int k = 0; k < TOPIC_NUM; ++k) {
        user_profile.topics.insert(user_profile.topics.end(), rand() % 1000);
      }
    
      // you must specified serialized field
      user_profile.__set_keywords(user_profile.keywords);
      // user_profile.__set_topics(user_profile.topics);
      user_profile_storage_->PutUserProfile(user_profile.uid, user_profile);
  }

  // test increment
  for (int i = 0; i < USER_NUM; ++i) {
      idl::UserProfile user_profile;
      char tmp[10];
      sprintf(tmp, "%d", i);
      user_profile.uid = "user_" + std::string(tmp);
      for (int64_t j = 0; j < KEYWORD_NUM; ++j) {
        user_profile.keywords[j] = rand() % 10000; 
        LOG(INFO) << "PutUserProfile Increment " << user_profile.uid <<" : keyword ---->" \
                  << "[" << j << "," << user_profile.keywords[j] << "]";
      }
      for (int k = 0; k < TOPIC_NUM; ++k) {
        user_profile.topics.insert(user_profile.topics.end(), rand() % 1000);
      }
    
      // you must specified serialized field
      user_profile.__set_keywords(user_profile.keywords);
      // user_profile.__set_topics(user_profile.topics);
      user_profile_storage_->PutUserProfile(user_profile.uid, user_profile, true);
  }
}

TEST_F(UserProfileStorageTest, GetUserProfile) {
  LOG(INFO) << "start get user profile";
  for (int i = 0; i < USER_NUM; ++i) {
      idl::UserProfile user_profile;
      char tmp[10];
      sprintf(tmp, "%d", i);
      std::string uid = "user_" + std::string(tmp);
      user_profile_storage_->GetUserProfile(uid, &user_profile);
      for (std::map<int64_t, int32_t>::const_iterator it =
           user_profile.keywords.begin();
           it != user_profile.keywords.end(); ++it) {
        LOG(INFO) << "GetUserProfile " << user_profile.uid <<" : keyword ---->" \
          << "[" << it->first << "," << it->second << "]";
      }
  }
}

}
