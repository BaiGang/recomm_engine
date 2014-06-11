#ifndef _RECOMM_ENGINE_IPLUGIN_H_
#define _RECOMM_ENGINE_IPLUGIN_H_

#include "recomm_engine_types.h"

namespace recomm_engine {

class IAlgoPlugin {
 public:
  virtual ~IAlgoPlugin() {};

  virtual int Initialize(const char* configuration_file) = 0;
  virtual int Uninitialize() = 0;
  virtual int ReloadConfiguation(const char* configuration_file = NULL) = 0;

  /** The ranking interface.
   * @param user_profile The UserProfile fetched from UPS.
   * @param candidates The candidates retrieved from the index.
   * @param results The final ranked and filtered results, each of which is a id in string format.
   * @param use_debug Trigger filling debug info in the results. Default is not to use debug.
   * All algo logics are opaque in the implementations. Logging, error handling are encapsulated.
  */
  virtual int Rank(const idl::UserProfile& user_profile,
                   const std::vector<idl::StoryProfile>& candidates,
                   std::vector<idl::RecommendationResult>* results,
                   bool use_debug = false) = 0;

  virtual const std::string& GetName() = 0;
};

}  // namespace recomm_engine

#endif  // _RECOMM_ENGINE_IPLUGIN_H_
