#include "story_profile_storage.h"
#include "leveldb/db.h"
#include "leveldb/cache.h"
#include "gflags/gflags.h"
#include "util/base/thrift_util.h"
#include "recomm_engine_types.h"

DEFINE_int32(sps_cache_size, 1048576*512, "Size in Bytes of the leveldb cache.");

namespace recomm_engine {

StoryProfileStorage::StoryProfileStorage() {
}

StoryProfileStorage::~StoryProfileStorage() {
}

bool StoryProfileStorage::Initialize(const std::string &filename) {
  db_.reset(new LocalDict(filename,
                          FLAGS_sps_cache_size));
  if (NULL == db_)
    return false;
  return true;
}

//add a story                                                
bool StoryProfileStorage::AddProfile(int64_t key, const idl::StoryProfile& value) {     
  std::string serialized_value = util::ThriftToString(&value);
  if (!db_->Put(leveldb::Slice(reinterpret_cast<char*>(&key), sizeof(int64_t)),
          serialized_value)) {
    LOG(ERROR) << "Failed Adding story profile for [" << key << "]";
    return false;
  }
  return true;
}

//get a story
bool StoryProfileStorage::GetProfile(int64_t key, idl::StoryProfile* value) {
  std::string serialized_value;
  int status = db_->Get(leveldb::Slice(reinterpret_cast<char*>(&key), sizeof(int64_t)),
                        &serialized_value);

  if (0 == status) {
    if (!util::StringToThrift(serialized_value, value)) {
      LOG(ERROR) << "Failed deserialization. ";
      return false;
    }
  } else if (1 == status) {
    value->story_id = "";
    return false;
  } else {
    return false;
  }

  return true;
}

// FIXME(baigang): To add the real iterator.
StoryProfileStorage::Iterator* StoryProfileStorage::GetIterator(bool reverse) {
  // return new Iterator(db_.get(), reverse);
  return NULL;
}

StoryProfileStorage StoryProfileStorage::instance_;

//////////////////////////////////////////////

StoryProfileStorage::Iterator::Iterator(::leveldb::DB* db, bool reverse) 
  : reverse_(reverse){
  iterator_.reset(db->NewIterator(leveldb::ReadOptions()));
  if (reverse_) {
    iterator_->SeekToLast();
  } else {
    iterator_->SeekToFirst();
  }
}

StoryProfileStorage::Iterator::~Iterator() {
}

bool StoryProfileStorage::Iterator::Next() {
  if (reverse_) {
    iterator_->Prev();
  } else {
    iterator_->Next();
  }
  return iterator_->Valid();
}

::leveldb::Slice StoryProfileStorage::Iterator::Key() {
  return iterator_->key();
}

::leveldb::Slice StoryProfileStorage::Iterator::Value() {
  return iterator_->value();
}

}  // namespace recomm_engine

