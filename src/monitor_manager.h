#ifndef _RECOMM_ENGINE_MONITOR_MANAGER_H_
#define _RECOMM_ENGINE_MONITOR_MANAGER_H_
#include "common.h"
#include <boost/atomic.hpp>

namespace recomm_engine {

  /**
   * A singleton class that encapsulates metrics for reporting and monitoring.
   */
class MonitorManager {

 public:
  inline static MonitorManager* Instance() {
    return &instance_;
  }

  virtual ~MonitorManager();

  // metrics for monitoring and reporting
  boost::atomic<int> num_processed_requests_;
  boost::atomic<int> num_error_responses_;
  boost::atomic<int> num_recommended_results_;
	boost::atomic<int> num_indexed_stories_current_;
	boost::atomic<int> num_indexed_stories_total_;


 private:
  static MonitorManager instance_;
  MonitorManager();
  DISALLOW_COPY_AND_ASSIGN(MonitorManager);
};

}  // namespace recomm_engine

#endif // _RECOMM_ENGINE_MONITOR_MANAGER_H_
