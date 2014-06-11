// Copyright 2013 Sina Inc. All rights reserved.
// Author: sijia2@staff.sina.com.cn (Sijia Yu)
// Description: index reader

#ifndef SRC_RETRIEVAL_RESULT_SET_H_
#define SRC_RETRIEVAL_RESULT_SET_H_

#include <vector>
#include "./recomm_engine_types.h"
#include "./recomm_engine_constants.h"

namespace recomm_engine {
namespace retrieving {
class result_set {
 public:
        result_set();
        ~result_set();

        int init();
        int dump();

 protected:
        struct entry {
                bool operator < (const entry &right) const {
                    return this->score > right.score;
                }
                int64_t  story_id;
                double   score;
        };

 public:
        entry doc(int index) {
              return m_docs[index];
        }

        inline void insert_doc(int64_t story_id, double score) {
                entry temp;
                temp.story_id = story_id;
                temp.score = score;
                m_docs.push_back(temp);
                ++m_doc_num;
        }
        inline int doc_num() const {
                return m_doc_num;
        }
        inline void set_max_num(int num) {
                m_max_num = num;
        }

 protected:
        std::vector<entry> m_docs;

        int m_doc_num;
        int m_max_num;
};
}  // namespace retrieving
}  // namespace recomm_engine
#endif  // SRC_RETRIEVAL_RESULT_SET_H_

