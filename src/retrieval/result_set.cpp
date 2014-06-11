// Copyright 2013 Sina Inc. All rights reserved.
// Author: sijia2@staff.sina.com.cn (Sijia Yu)
// Description: index reader

#include "./result_set.h"
#include <algorithm>

namespace recomm_engine {
namespace retrieving {
result_set::result_set() {
        init();
}

result_set::~result_set() {
}

int result_set::init() {
        m_doc_num = 0;
        m_max_num = 0;
        return 0;
}

int result_set::dump() {
    std::sort(m_docs.begin(), m_docs.end());
    return m_doc_num;
}
}  // namespace retrieving
}  // namespace recomm_engine
