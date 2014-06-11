// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing  Bai)
// Description: Posting list Iterato

#ifndef SRC_INDEXING_ITERATOR_H_
#define SRC_INDEXING_ITERATOR_H_

#include <stdint.h>
#include "./posting.h"

namespace recomm_engine {
namespace indexing {

/*
 * Iterator for iterating the posting list
*  Sample Usage:
*  while (it->HasValue()) {
*    Posting p = it->GetValue();
*    ... 
*    it->Next();
*  }
*  NOTE!!!: Please use iterator with strict order as above.
*  Any illegal operation may result in undefined behaviour
*/
class PostingIterator {
  public:
    PostingIterator() {}
    virtual ~PostingIterator() {}

    // public interfaces
    // If the iterator pointing to a valid posting
    virtual bool HasValue() = 0;
    // Get posting. Only used after pass the HasValue test
    // The GetValue can be called only once after HasValue
    virtual Posting GetValue() = 0;
    // Move to next posting
    virtual void Next() = 0;
};
// An implementation of iterator
class PostingIteratorImpl : public PostingIterator {
  public:
    // Constructor and destructor
    explicit PostingIteratorImpl(const PostingList& _plist);
    ~PostingIteratorImpl();
    // public interface
    bool HasValue();
    Posting GetValue();
    void Next();
  private:
    const util::ListNode<Posting>* GetMark();
  private:
    const PostingList& plist_;
    const util::ListNode<Posting>* cur_;
    bool has_value_;
};

}  // namespace indexing
}  // namespace recomm_engine
#endif  // SRC_INDEXING_ITERATOR_H_
