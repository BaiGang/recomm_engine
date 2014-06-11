// Copyright 2013 Sina Inc. All rights reserved.
// Author: sijia2@staff.sina.com.cn (Sijia Yu)
// Description: rank per doc
#include "./ranker.h"
namespace recomm_engine {
namespace retrieving {
Ranker::Ranker() {
}

Ranker::~Ranker() {
}

double Ranker::compute_rank(RankerDoc* doc) {
  double score = 0.0;
  return score;
}

int Ranker::close() {
  return 0;
}
}  // namespace retrieving
}  // namespace recomm_engine
