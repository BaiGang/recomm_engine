// Copyright 2013 Sina Inc. All rights reserved.
// Author: sijia2@staff.sina.com.cn (Sijia Yu)
// Description: rank per doc

#ifndef  SRC_RETRIEVAL_TYPES_H_
#define  SRC_RETRIEVAL_TYPES_H_
#include <stdio.h>
#include <vector>

namespace recomm_engine {
namespace retrieving {
struct KeyOfDoc {
  int64_t keyword_id;
  double weight_in_doc;
};

struct DocInfo {
  int64_t story_id;
  std::vector<KeyOfDoc> key_info;
};
}  // namespace retrieving
}  // namespace recomm_engine
#endif  // SRC_RETRIEVAL_TYPES_H_
