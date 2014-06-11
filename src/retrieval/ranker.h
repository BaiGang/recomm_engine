// Copyright 2013 Sina Inc. All rights reserved.
// Author: sijia2@staff.sina.com.cn (Sijia Yu)
// Description: rank per doc

#ifndef  SRC_RETRIEVAL_RANKER_H_
#define  SRC_RETRIEVAL_RANKER_H_

#include<vector>
#include "./types.h"

namespace recomm_engine {
namespace retrieving {

typedef DocInfo RankerDoc;
class Ranker {
 public:
    Ranker();
    ~Ranker();

 public:
        double compute_rank(RankerDoc* doc);
        int close();
};
}  // namespace retrieving
}  // namespace recomm_engine
#endif  // SRC_RETRIEVAL_RANKER_H_
