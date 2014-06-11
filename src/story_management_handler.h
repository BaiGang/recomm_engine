#ifndef _RECOMM_ENGINE_STORY_MANAGEMENT_HANDLER_H_
#define _RECOMM_ENGINE_STORY_MANAGEMENT_HANDLER_H_
#include "StoryManagement.h"
#include "glog/logging.h"

namespace recomm_engine {

class StoryManagementHandler : virtual public idl::StoryManagementIf {
 public:
  StoryManagementHandler();
  virtual ~StoryManagementHandler();

  /** Add a story into the db.
   *
   *    It will and the story profile into both the inverted index and the index.
   */
  void add_story(idl::StoryAddingResponse& _return, const idl::StoryAddingRequest& request);

};

}  // namespace recomm_engine

#endif  // _RECOMM_ENGINE_STORY_MANAGEMENT_HANDLER_H_

