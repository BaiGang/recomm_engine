// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing  Bai)
// Description: Posting structure

#ifndef SRC_INDEXING_POSTING_H_
#define SRC_INDEXING_POSTING_H_

#include <boost/shared_ptr.hpp>
#include "./base.h"
#include "util/containers/lockless_list.h"

namespace recomm_engine {
namespace indexing {
/* 
 * The Postings in invert index
*/
class Posting {
  public:
    LocalDocID local_docid;  // Local docid
    float weight;  // Term weight in this doc
    int32_t timestamp;  // The time when doc pushed
};


/* 
 * list of postings
*/
class PostingList {
  public:
    // Constructor and destructor
    PostingList();
    ~PostingList();
    // Get front posting
    const util::ListNode<Posting>* GetFakeFront() const;
    // Get end mark
    const util::ListNode<Posting>* GetEndMark()const;
    // Set end mark
    void SetEndMark(const util::ListNode<Posting>* mark);
    // Add poting into posting list
    void AddPosting(util::ListNode<Posting>* p);
    // Allocate an empty node
    util::ListNode<Posting>* AllocatePosting();
    // Erase postings after specified position
    void EraseAfter(const util::ListNode<Posting>* p);
    // Get Size of the posting list, time O(n)
    size_t Size();
  private:
    boost::shared_ptr<util::LocklessList<Posting> > inner_list_;
    util::ListNode<Posting>* end_mark_;
};

}  // namespace indexing
}  // namespace recomm_engine
#endif  // SRC_INDEXING_POSTING_H_
