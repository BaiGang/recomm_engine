#include <iostream>
#include <thrift/processor/PeekProcessor.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "city.h"
#include "thrift_service.h"
#include "algo_plugin_manager.h"
#include "recomm_engine_handler.h"
#include "story_management_handler.h"
#include "story_profile_storage.h"
#include "user_profile_storage.h"

DEFINE_int32(story_management_port, 6100, "Port for story management service.");
DEFINE_int32(recommendation_port, 6300, "Port for recommendation service.");
DEFINE_string(story_profile_storage_path, "/tmp/sps.leveldb", "select where to save leveldb local file");
DEFINE_string(rank_plugin_config_path, "./test/algo_plugin_manager_test.conf", "Path to the config file for bucket/algo.");

void InitEnvironment() {
  CHECK(recomm_engine::AlgoPluginManager::GetInstance()->
        Initialize(FLAGS_rank_plugin_config_path));
  CHECK(recomm_engine::StoryProfileStorage::GetInstance()->
        Initialize(FLAGS_story_profile_storage_path));
}

int main(int argc, char* argv[]) {
  using namespace google;
  using namespace boost;
  using namespace recomm_engine;
  using namespace ::apache::thrift;

  InitGoogleLogging(argv[0]);
  ParseCommandLineFlags(&argc, &argv, true);
  
  //LOG(INFO) << "StoryManager Initialize completed.";
  InitEnvironment();
 
  shared_ptr<RecommEngineHandler> recomm_engine_handler(
      new RecommEngineHandler());
  shared_ptr<TProcessor> processor(
      new idl::RecommEngineProcessor(recomm_engine_handler));
  ThriftService< RecommEngineHandler,
                 TProcessor,
                 server::TThreadedServer> recomm_engine_service(
    recomm_engine_handler, processor, FLAGS_recommendation_port);

  LOG(INFO) << "RecommEngine service created.";

  shared_ptr<StoryManagementHandler> story_management_handler(
      new StoryManagementHandler());
  shared_ptr<TProcessor> sm_processor(
      new idl::StoryManagementProcessor(story_management_handler));
  ThriftService< StoryManagementHandler,
                 TProcessor,
                 server::TThreadedServer> story_management_service(
    story_management_handler, sm_processor, FLAGS_story_management_port);

  LOG(INFO) << "StoryManagement service created.";

  // start the document db service as a background thread
  story_management_service.ServeDetached();
  
  // start the recommendation engine service
  recomm_engine_service.Serve();
 
  recomm_engine_service.End();
  story_management_service.End();

  return 0;
}

