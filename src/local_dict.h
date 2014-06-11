#ifndef _UTIL_LOCAL_DICT_H_
#define _UTIL_LOCAL_DICT_H_
#include "common.h"
#include <string>
#include <boost/smart_ptr.hpp>

namespace leveldb {
class DB;
class Slice;
}

namespace recomm_engine {

/** \brief A local persistent dict.
 */
class LocalDict {
 public:
  explicit LocalDict(const std::string& local_path, int memory = 2 * 1024 * 1024);
  virtual ~LocalDict();

  bool Valid();

  bool Put(const leveldb::Slice& key,
           const leveldb::Slice& value);

  //*
  // return value: 
  //      0 - success
  //      1 - not found
  //      2 - error
  int Get(const leveldb::Slice& key,
           std::string* value);

  // data file consists of lines of "key value".
  // FIXME(baigang): no space allowed in key or value
  bool Bootstrap(const std::string& filename);

  bool Purge();

 protected:
  bool is_valid_;
  boost::scoped_ptr<leveldb::DB> persistence_;
};

}  // namespace recomm_engine

#endif  // _UTIL_LOCAL_DICT_H_

