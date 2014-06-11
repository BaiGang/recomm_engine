#include <dlfcn.h>
#include <iostream>
#include <fstream>
#include <boost/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include "algo_plugin_manager.h"
#include "ialgo_plugin.h"
#include "city.h"

DEFINE_int32(reloading_period_seconds, 3600, "Num of seconds between two reloadings of the configs.");

namespace {
typedef recomm_engine::IAlgoPlugin *(*create_plugin_function_t)();

recomm_engine::IAlgoPlugin* GetPluginFromDylib(
    const std::string& dylib_file,
    const std::string& symbol_name,
    const std::string& config) {

  LOG(INFO) << "lib name: " << dylib_file << ", symbol name: " << symbol_name; 
 
  void* so_handle = dlopen(dylib_file.c_str(), RTLD_NOW);
  if (NULL == so_handle) {
    LOG(ERROR) << "Failed dlopen " << dylib_file << ", due to " << dlerror();
    return NULL;
  }
  create_plugin_function_t create_plugin_function;
  create_plugin_function = (create_plugin_function_t)dlsym(so_handle, symbol_name.c_str());
  if (NULL == create_plugin_function || NULL == *create_plugin_function) {
    LOG(ERROR) << "Failed load symbol <" << symbol_name << "> for " << dylib_file << ", due to " << dlerror();
    return NULL;
  }
  recomm_engine::IAlgoPlugin* result = (*create_plugin_function)();
  if (NULL == result) {
    LOG(ERROR) << "Failed creating plugin object using <" << dylib_file << ">, due to " << dlerror();
    return NULL;
  }

  int status = result->Initialize(config.c_str());
  if (0 != status) {
    LOG(ERROR) << "Failed configuring plugin [" << config << "], status is " << status;
    delete result;
    return NULL;
  }
  
  // XXX(baigang): Do not dlclose, otherwise we will miss all function definitions and encounter segment faults...
  // dlclose(so_handle);
  return result;
}
}  // unnamed namespace

namespace recomm_engine {

AlgoPluginManager* AlgoPluginManager::instance_ = new AlgoPluginManager;

AlgoPluginManager::AlgoPluginManager()
  : current_index_(0) {
}

AlgoPluginManager::~AlgoPluginManager() {

}

// @static 
void AlgoPluginManager::PeriodicalReloadingThread(AlgoPluginManager* handle) {
  while (true) {
    boost::this_thread::sleep(boost::posix_time::seconds(FLAGS_reloading_period_seconds));
    LOG_IF(ERROR, !handle->LoadPlugins()) << "Failed reloading plugin configs.";
  }
}

bool AlgoPluginManager::Initialize(const std::string& config_file) {
  config_file_ = config_file;
  if (!LoadPlugins()) {
    LOG(ERROR) << "Initialization failed due to failure to load config file " << config_file;
    return false;
  }
  boost::thread config_reloading_thread(&(AlgoPluginManager::PeriodicalReloadingThread), this);
  return true;
}

bool AlgoPluginManager::LoadPlugins() {
  // use the write buffer
  int index = 1 - current_index_;
  plugins_[index].clear();
  std::ifstream config_file(config_file_.c_str());
  if (!config_file.good()) {
    LOG(ERROR) << "Failed open config file [" << config_file_ << "]";
    return false;
  }
  boost::program_options::options_description desc("bucket configs");
  int default_bucket = 0;
  std::string buckets;
  std::string plugin_path;
  desc.add_options()
    ("num_buckets",    boost::program_options::value<int>(&num_plugins_[index]),
         "Num of traffic buckets.")
    ("default_bucket", boost::program_options::value<int>(&default_bucket)->default_value(0),
         "The default bucket, used when no bucket conf is specified.")
    ("plugin_path",    boost::program_options::value<std::string>(&plugin_path),
         "The path to the plugin libs.")
    ("buckets",        boost::program_options::value<std::string>(&buckets)->multitoken(),
         "A series of bucket specifications.")
  ;
  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_config_file(config_file, desc), vm);
  config_file.close();
  boost::program_options::notify(vm);

  LOG(INFO) << "Buckets specification is: {" << buckets << "}";
  
  boost::char_separator<char> space_sep(" \t");
  boost::char_separator<char> colon_sep(":");
  boost::tokenizer<boost::char_separator<char> > bucket_items(buckets, space_sep);
  for (boost::tokenizer<boost::char_separator<char> >::iterator it = bucket_items.begin();
       it != bucket_items.end(); ++it) {
    boost::tokenizer<boost::char_separator<char> > fields(*it, colon_sep);
    boost::tokenizer<boost::char_separator<char> >::iterator field = fields.begin();
  
    // 1. bucket id
    int bucket_id = 0;
    try {
      bucket_id = boost::lexical_cast<int>(*field);
    } catch (boost::bad_lexical_cast &) {
      LOG(ERROR) << "Failed reading bucket_id on \"" << *it << "\""; 
      return false;
    }
 
    // 2. bucket name
    std::string bucket_name = *(++field);

    // 3. bucket plugin
    std::string plugin_name = *(++field);
    std::string plugin_libfile = plugin_path + "/lib" + plugin_name + ".so";
    std::string plugin_config  = plugin_path + "/lib" + plugin_name + ".conf";
    std::string plugin_symbol  = "__create_" + plugin_name;

    LOG(INFO) << "Loading config for bucket [" << bucket_id << "].";
    
    boost::shared_ptr<IAlgoPlugin> this_plugin(GetPluginFromDylib(
        plugin_libfile, plugin_symbol, plugin_config));

    if (NULL == this_plugin) {
      LOG(ERROR) << "Failed loading plugin [" << bucket_id << "-" << plugin_name << "]";
      return false;
    }

    plugins_[index][bucket_id] = this_plugin;
    LOG(INFO) << " Name [" << bucket_name << "].";
    LOG(INFO) << " Lib file: [" << plugin_libfile << "].";
    LOG(INFO) << " Lib interface symbol name file: [" << plugin_symbol << "].";
    LOG(INFO) << " Config file: [" << plugin_config << "]";
    LOG(INFO) << "Done loading for bucket " << bucket_id;
  }

  // FIXME(baigang): use safe access
  LOG(INFO) << "Default bucket is: " << default_bucket;  
  default_plugin_[index] = plugins_[index][default_bucket];
  LOG(INFO) << "Size of effective plugins is " << plugins_[index].size();

  // swap read/write buffer 
  current_index_ = index;
  return true;
}

const boost::shared_ptr<IAlgoPlugin> AlgoPluginManager::Select(uint64_t id) {
  int index = current_index_;
  int mod = id % num_plugins_[index];
  boost::unordered_map<int, boost::shared_ptr<IAlgoPlugin> >::const_iterator iter = plugins_[index].find(mod);
  if (plugins_[index].end() == iter) {
    return default_plugin_[index];
  }
  return iter->second;
}

const boost::shared_ptr<IAlgoPlugin> AlgoPluginManager::Select(const std::string& id) {
  return Select(CityHash64(id.data(), id.length()));
}

}  // namespace recomm_engine

