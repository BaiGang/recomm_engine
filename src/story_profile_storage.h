#ifndef _RECOMM_ENGINE_STORY_PROFILE_STORAGE_H_
#define _RECOMM_ENGINE_STORY_PROFILE_STORAGE_H_

#include <string>
#include <boost/smart_ptr.hpp>
#include "local_dict.h"
#include "glog/logging.h"

namespace leveldb {
class DB;
class Iterator;
}

namespace recomm_engine {

namespace idl {
class StoryProfile;
}

class StoryProfileStorage {
 public:
  class Iterator;

  virtual ~StoryProfileStorage();
  // Get the singleton instance. Synchnorization will be handled by leveldb per se.
  inline static StoryProfileStorage* GetInstance() {
    return &instance_;
  }

  // open/create a db
  // should be synchronized
  bool Initialize(const std::string& name);

  // threading safe getters and mutaters
  
  // gets a thread-local iterator
  // Caller thould manage the resource and free it after use.
  Iterator* GetIterator(bool reverse = false);
  
  // add a story
  bool AddProfile(int64_t key, const idl::StoryProfile& value);
  // update a story
  bool GetProfile(int64_t key, idl::StoryProfile* value);

 protected:
  boost::scoped_ptr<LocalDict> db_;
  static StoryProfileStorage instance_;
 private:
  StoryProfileStorage();
};


/**
 * A wrapper for the leveldb iterator.
 */
class StoryProfileStorage::Iterator {
 public:

  virtual ~Iterator();

  //! Proceed to next record, returning false iff the iterator
  //  is invalid.
  // Because keys are sorted alpha-betically and the initial 
  // characters are the time string, we actually iterate over
  // the keys reversely, i.e newest first.
  bool Next();

  //! Key buffer of the current record.
  ::leveldb::Slice Key();

  //! Value buffer of the current record.
  ::leveldb::Slice Value();

 protected:

  explicit Iterator(::leveldb::DB* db, bool reverse = false);
  boost::scoped_ptr< ::leveldb::Iterator> iterator_;
  bool reverse_;

 private:
  Iterator(const Iterator&);
  void operator = (const Iterator&);
 
  friend class StoryProfileStorage;
};

}  // namespace recomm_engine

#endif  // _RECOMM_ENGINE_STORY_MANAGER_H_

