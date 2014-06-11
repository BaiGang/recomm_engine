#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include "leveldb/slice.h"
#include "gtest.h"
#include "local_dict.h"

namespace {
const char DB_PATH[] = "./test/local_dict_test.db";
}

namespace recomm_engine {

class LocalDictTest : public ::testing::Test {
 public:
  virtual void SetUp();
  virtual void TearDown();
 protected:
  boost::scoped_ptr<LocalDict> dict_; 
};

void LocalDictTest::SetUp() {
  boost::filesystem::remove_all(boost::filesystem::path(DB_PATH));
  dict_.reset(new LocalDict(DB_PATH));
  ASSERT_TRUE(dict_ != NULL);
}

void LocalDictTest::TearDown() {
  dict_.reset();
  boost::filesystem::remove_all(boost::filesystem::path(DB_PATH));
}

TEST_F(LocalDictTest, PutGetData) {
  EXPECT_TRUE(dict_->Put(leveldb::Slice("testingkey1"), leveldb::Slice("testingvalue1")));
  EXPECT_TRUE(dict_->Put(leveldb::Slice("2testingkey"), leveldb::Slice("2testingvalue")));
  EXPECT_TRUE(dict_->Put(leveldb::Slice("testing0123"), leveldb::Slice("testingvalue1234")));
  EXPECT_TRUE(dict_->Put(leveldb::Slice("testingkey1"), leveldb::Slice("testingvalue1")));
 
  std::string value;
  EXPECT_EQ(0, dict_->Get(leveldb::Slice("2testingkey"), &value));
  EXPECT_EQ(value, leveldb::Slice("2testingvalue"));

  EXPECT_EQ(0, dict_->Get(leveldb::Slice("testingkey1"), &value));
  EXPECT_EQ(value, leveldb::Slice("testingvalue1"));
  
  EXPECT_TRUE(dict_->Put(leveldb::Slice("999testingk"), leveldb::Slice("1")));
  
  EXPECT_EQ(0, dict_->Get(leveldb::Slice("testing0123"), &value));
  EXPECT_EQ(value, leveldb::Slice("testingvalue1234"));
  
  EXPECT_EQ(0, dict_->Get(leveldb::Slice("999testingk"), &value));
  EXPECT_EQ(value, "1");
}

TEST_F(LocalDictTest, BootstrapData) {
  EXPECT_TRUE(dict_->Bootstrap("./test/local_dict_test.bootstrap"));
  std::string value;
 
  EXPECT_EQ(0, dict_->Get(leveldb::Slice("testingkey2"), &value));
  EXPECT_EQ(value, leveldb::Slice("testingvalue2"));
  
  EXPECT_EQ(0, dict_->Get(leveldb::Slice("testingkey7"), &value));
  EXPECT_EQ(value, leveldb::Slice("testingvalue7"));
} 

TEST_F(LocalDictTest, ReuseDict) {

  boost::filesystem::remove_all("./ReuseDict.test.db");
  boost::scoped_ptr<LocalDict> dict1(new LocalDict("./ReuseDict.test.db"));
  EXPECT_TRUE(dict1->Put(leveldb::Slice("aaabbbtestccc"), leveldb::Slice("111-222-333-444")));
  std::string test_value1;
  EXPECT_EQ(0, dict1->Get(leveldb::Slice("aaabbbtestccc"), &test_value1));
  EXPECT_EQ(test_value1, leveldb::Slice("111-222-333-444"));
  dict1.reset();

  boost::scoped_ptr<LocalDict> dict2(new LocalDict("./ReuseDict.test.db"));
  EXPECT_EQ(0, dict2->Get(leveldb::Slice("aaabbbtestccc"), &test_value1));
  EXPECT_EQ(test_value1, leveldb::Slice("111-222-333-444"));
  dict2.reset();

  boost::filesystem::remove_all("./ReuseDict.test.db");
}

}  // namespace 


