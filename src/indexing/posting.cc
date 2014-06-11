// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing  Bai)
// Description: Posting structure

#include "./posting.h"

using util::ListNode;
using boost::shared_ptr;
using util::LocklessList;

namespace recomm_engine {
namespace indexing {

// Constructor
PostingList::PostingList() {
  AtomicPointerAssgin(&end_mark_,
                      reinterpret_cast<ListNode<Posting>*>(NULL));
  inner_list_ = shared_ptr<LocklessList<Posting> >(
    new LocklessList<Posting>(NULL));
}

// Destructor
PostingList::~PostingList() {
}

// Get the front node of the posting list
const ListNode<Posting>* PostingList::GetFakeFront() const {
  const ListNode<Posting>* front;
  inner_list_->Front(&front);
  return front;
}

// Get end mark
const ListNode<Posting>* PostingList::GetEndMark() const {
  const ListNode<Posting> * mark;
  AtomicPointerAssgin(&mark,
                      reinterpret_cast<const ListNode<Posting>*>(end_mark_));
  return mark;
}
// Set end mark for the posting list
void PostingList::SetEndMark(const ListNode<Posting>* mark) {
  AtomicPointerAssgin(&end_mark_,
  const_cast<ListNode<Posting>*>(mark));
}

// Add posting ahead into posting list
void PostingList::AddPosting(ListNode<Posting>* p) {
  inner_list_->PushFront(p);
}
// Allocate a new posting list
ListNode<Posting>* PostingList::AllocatePosting() {
  ListNode<Posting>* p = inner_list_->Allocate();
  p->next = NULL;
  // p->data = new Posting();
  return p;
}
size_t PostingList::Size() {
  const ListNode<Posting>* cur = GetFakeFront();
  if (cur == GetEndMark()) {
    return 0;
  } else {
    size_t size = 0;
    while (cur != NULL && cur->next != NULL && cur != GetEndMark()) {
      ++size;
      cur = cur->next;
    }
    return size;
  }
}
// Erase the postings after given posting in posting list
void PostingList::EraseAfter(const ListNode<Posting>* p) {
  if (p) {
    inner_list_->EraseAfter(p);
  }
}
}  // namespace indexing
}  // namespace recomm_engine
