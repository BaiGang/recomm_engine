#ifndef _RECOMM_ENGINE_ALGO_PLUGIN_MANAGER_H_
#define _RECOMM_ENGINE_ALGO_PLUGIN_MANAGER_H_
#include <string>
#include <boost/atomic.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/unordered_map.hpp>
#include "common.h"

namespace recomm_engine {

class AlgoPluginManagerTest_ReloadConfig_Test;

class IAlgoPlugin;
/*
 * @class AlgoPluginManager wraps the selection, loading and updating of algo plugins.
 *  Each algo plugin is essentially a shared object lib. AlgoPluginManager loads the 
 *  symbols via dlopen and dlsymbol.

 *  We instantiate one object .
 */
class AlgoPluginManager {
 public:
  virtual ~AlgoPluginManager();

  static inline AlgoPluginManager* GetInstance() {
    return instance_;
  }
  bool Initialize(const std::string& config_file);
  
  const boost::shared_ptr<IAlgoPlugin> Select(uint64_t id);
  const boost::shared_ptr<IAlgoPlugin> Select(const std::string& id);

 protected:
  // Singeleton instance.
  static AlgoPluginManager* instance_;

  boost::atomic<int> current_index_;  // separate read/write instance
  boost::unordered_map<int,
     boost::shared_ptr<IAlgoPlugin> > plugins_[2];
  boost::shared_ptr<IAlgoPlugin> default_plugin_[2];
  int num_plugins_[2];

  std::string config_file_;

  static void PeriodicalReloadingThread(AlgoPluginManager* handle);
  bool LoadPlugins();

  friend class AlgoPluginManagerTest_ReloadConfig_Test;

 private:
  AlgoPluginManager();
  DISALLOW_COPY_AND_ASSIGN(AlgoPluginManager);
};

}  // namespace recomm_engine

#endif  // _RECOMM_ENGINE_ALGO_PLUGIN_MANAGER_H_

