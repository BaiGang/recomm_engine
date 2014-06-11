#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include "gtest.h"
#include "algo_plugin_manager.h"
#include "ialgo_plugin.h"
#include "city.h"
#include "common.h"

namespace recomm_engine {

class AlgoPluginManagerTest : public ::testing::Test {
 public:
  AlgoPluginManagerTest();
  virtual ~AlgoPluginManagerTest();
  virtual void SetUp();
  virtual void TearDown();
 protected:

};

AlgoPluginManagerTest::AlgoPluginManagerTest() {
}

AlgoPluginManagerTest::~AlgoPluginManagerTest() {
}

void AlgoPluginManagerTest::SetUp() {
  ASSERT_TRUE(AlgoPluginManager::GetInstance()->Initialize("./test/algo_plugin_manager_test.conf"));
}

void AlgoPluginManagerTest::TearDown() {

}

TEST_F(AlgoPluginManagerTest, LoadConfigs) {
  // Already checked in the SetUp()
  EXPECT_TRUE(true);
}

TEST_F(AlgoPluginManagerTest, GetPluginByHashcode) {
  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin1 =
    AlgoPluginManager::GetInstance()->Select(0);
  ASSERT_TRUE(NULL != plugin1);
  EXPECT_STREQ("TestPlugin", plugin1->GetName().c_str());

  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin2 =
    AlgoPluginManager::GetInstance()->Select(2);
  ASSERT_TRUE(NULL != plugin2);
  EXPECT_STREQ("TestPluginAAA", plugin2->GetName().c_str());

}

TEST_F(AlgoPluginManagerTest, GetPluginByString) {
  LOG(INFO) << "CITYHASH OF aabbcc is " << CityHash64("aabbcc", 6)%4;
  LOG(INFO) << "CITYHASH OF f-16" << CityHash64("f-16", 4)%4;
  LOG(INFO) << "CITYHASH OF SINA is " << CityHash64("SINA", 4)%4;
  LOG(INFO) << "CITYHASH OF China is " << CityHash64("China", 5)%4;

  // cityhash64 of "f-16" mod 4 is 0
  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin1 =
    AlgoPluginManager::GetInstance()->Select("f-16");
  ASSERT_TRUE(NULL != plugin1);
  EXPECT_STREQ("TestPlugin", plugin1->GetName().c_str());

  // cityhash64 of "SINA" mod 4 is 2
  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin2 =
    AlgoPluginManager::GetInstance()->Select("SINA");
  ASSERT_TRUE(NULL != plugin2);
  EXPECT_STREQ("TestPluginAAA", plugin2->GetName().c_str());

  // cityhash64 of "aabbcc" mod 4 is 1
  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin3 =
    AlgoPluginManager::GetInstance()->Select("aabbcc");
  ASSERT_TRUE(NULL != plugin3);
  EXPECT_STREQ("TestPlugin", plugin3->GetName().c_str());
}

TEST_F(AlgoPluginManagerTest, DefaultPlugin) {
  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin1 =
    AlgoPluginManager::GetInstance()->Select(1);
  ASSERT_TRUE(NULL != plugin1);
  EXPECT_STREQ("TestPlugin", plugin1->GetName().c_str());

  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin2 =
    AlgoPluginManager::GetInstance()->Select(3);
  ASSERT_TRUE(NULL != plugin2);
  EXPECT_STREQ("TestPlugin", plugin2->GetName().c_str());

  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin3 =
    AlgoPluginManager::GetInstance()->Select(2);
  ASSERT_TRUE(NULL != plugin3);
  EXPECT_STREQ("TestPluginAAA", plugin3->GetName().c_str());
}

TEST_F(AlgoPluginManagerTest, ReloadConfig) {
  // @mutation: friend of AlgoPluginManager
  
  // The original config is loaded
  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin1 =
      AlgoPluginManager::GetInstance()->Select(1);
  ASSERT_TRUE(NULL != plugin1);
  EXPECT_STREQ("TestPlugin", plugin1->GetName().c_str());
  
  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin2 =
      AlgoPluginManager::GetInstance()->Select(2);
  ASSERT_TRUE(NULL != plugin2);
  EXPECT_STREQ("TestPluginAAA", plugin2->GetName().c_str());

  // Now load the updated config.
  // First replace the config, and back up the original one.
  boost::filesystem::path original_config("./test/algo_plugin_manager_test.conf");
  boost::filesystem::path updated_config("./test/algo_plugin_manager_test_b.conf");
  boost::filesystem::path backup_config_path("./test/algo_plugin_manager_test_c.conf");

  std::cerr << "Moving " << original_config << " into " << backup_config_path << std::endl;
  boost::filesystem::rename(original_config, backup_config_path);
  std::cerr << "Moving " << updated_config << " into " << original_config << std::endl;
  boost::filesystem::rename(updated_config, original_config);
 
  // Then load it
  AlgoPluginManager::GetInstance()->LoadPlugins();

  // Then check the plugins
  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin3 =
      AlgoPluginManager::GetInstance()->Select(1);
  EXPECT_TRUE(NULL != plugin3);
  EXPECT_STREQ("TestPluginAAA", plugin3->GetName().c_str());

  boost::shared_ptr< ::recomm_engine::IAlgoPlugin> plugin4 =
      AlgoPluginManager::GetInstance()->Select(3);
  EXPECT_TRUE(NULL != plugin4);
  EXPECT_STREQ("TestPlugin", plugin4->GetName().c_str());

  // Clear up. Put back the original and the updated configs.
  boost::filesystem::rename(original_config, updated_config);
  boost::filesystem::rename(backup_config_path, original_config);
}

}  // namespace recomm_engine

