#include "local_dict.h"
#include <fstream>
#include "leveldb/db.h"
#include "leveldb/cache.h"

namespace recomm_engine {

LocalDict::LocalDict(const std::string& local_path, int memory) {
  leveldb::Options options;
  options.create_if_missing = true;
  options.block_cache = leveldb::NewLRUCache(memory);
  leveldb::DB *db = NULL;
  leveldb::Status status = leveldb::DB::Open(options, local_path, &db);
  if (status.ok()) {
    LOG(INFO) << "Dict created successfully.";
    persistence_.reset(db);
  } else {
    LOG(ERROR) << "Dict creation failed!!!";
  }
}

LocalDict::~LocalDict() {
}

bool LocalDict::Valid() {
  return is_valid_;
}

bool LocalDict::Put(const leveldb::Slice& key,
                    const leveldb::Slice& value) {
  static leveldb::WriteOptions options;
  leveldb::Status status = persistence_->Put(options, key, value);
  if (status.ok()) {
    return true;
  } else {
    LOG(ERROR) << "Failed putting data - " << status.ToString();
    return false;
  }
}

int LocalDict::Get(const leveldb::Slice& key,
                    std::string* value) {
  static leveldb::ReadOptions options;
  leveldb::Status status = persistence_->Get(options, key, value);
  if (status.ok()) {
    return 0;
  } else if (status.IsNotFound()) {
    return 1;
  } else {
    LOG(ERROR) << "Failed getting data - " << status.ToString();
    return 2;
  }
}

bool LocalDict::Bootstrap(const std::string& filename) {
  Purge();
  std::fstream fin(filename.c_str(), std::ios_base::in);
  std::string key;
  std::string value;
  while (fin >> key >> value) {
    if (!Put(key, value)) {
      LOG(ERROR) << "Bootstrapping dict failed! ";
      return false;
    }
  }
  return true;
}

bool LocalDict::Purge() {
  // FIXME(baiang): implementations
  return true;
}

}  // namespace recomm_engine

