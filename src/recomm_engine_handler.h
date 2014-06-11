#ifndef _RECOMM_ENGINE_HANDLER_H_
#define _RECOMM_ENGINE_HANDLER_H_

#include "common.h"
#include <boost/scoped_ptr.hpp>
#include "RecommEngine.h"
#include "recomm_engine_types.h"

namespace recomm_engine {
namespace retrieving {
class RetrievalHandler;
}
class UserProfileStorage;

class RecommEngineHandler : virtual public idl::RecommEngineIf {
 public:
  RecommEngineHandler();
  virtual ~RecommEngineHandler(); 

  void query(idl::RecommendationResponse& _return, const idl::RecommendationRequest& request);

  /** facebook 303 interfaces
  */
  // FIXME(baigang): Add the organic implementation of those monitor/metrics interfaces!
  inline void getName(std::string& _return) {
    _return = "recommendation engine";
  }
  inline void getVersion(std::string& _return) {
    _return = "1";
  }
  inline void getStatusDetails(std::string _return) {
    _return = "ok";
  }
  inline ::facebook::fb303::fb_status getStatus() {return ::facebook::fb303::ALIVE;}
  inline void getStatusDetails(std::string& _return) {_return = "ok";}
  inline void getCounters(std::map<std::string, int64_t>& _return) {};
  inline int64_t getCounter(const std::string& key) {return 0;}
  inline void setOption(const std::string& key, const std::string& value) {
    return;
  }
  inline void getOption(std::string& _return, const std::string& key) {
    _return = "ok";
  }
  inline void getOptions(std::map<std::string, std::string> & _return) {
    return;
  }
  inline void getCpuProfile(std::string& _return, const int32_t profileDurationInSec) {
  }
  inline int64_t aliveSince() {return 0;}
  inline void reinitialize() {return;}
  inline void shutdown() {return;}

 protected:
  DISALLOW_COPY_AND_ASSIGN(RecommEngineHandler);
  boost::scoped_ptr<UserProfileStorage> user_profile_storage_;
  boost::scoped_ptr<retrieving::RetrievalHandler> retrieval_handler_;
};

}  // namespace recomm_engine

#endif  // _RECOMM_ENGINE_HANDLER_H_

