// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing  Bai)
// Description: Posting list Iterato

#include "./iterator.h"

using util::ListNode;

namespace recomm_engine {
namespace indexing {

PostingIteratorImpl::PostingIteratorImpl(
  const PostingList& _plist):plist_(_plist), has_value_(false) {
  cur_ = plist_.GetFakeFront();
  CHECK(cur_ != NULL);
}
PostingIteratorImpl::~PostingIteratorImpl() {
}
// whether can has next value
bool PostingIteratorImpl::HasValue() {
  if (cur_ == NULL || cur_->next == NULL) {
    has_value_ = false;
  } else {
    const ListNode<Posting>* mark = plist_.GetEndMark();
    if (mark == plist_.GetFakeFront()) {
      has_value_ = false;
    } else {
      if (cur_ == mark) {
        has_value_ = false;
      } else {
        has_value_ = true;
      }
    }
  }
  return has_value_;
}

// Get actual value
Posting PostingIteratorImpl::GetValue() {
  // safety check
  if (!has_value_) {
    throw "GetValue should be called after HasValue returning True";
  }
  // Do get value
  has_value_ = false;
  return *(cur_->next->data);
}

// Move to next value
void PostingIteratorImpl::Next() {
  // if (cur_->next != NULL) {
  //   cur_ = cur_->next;
  // } else {
  //   cur_ = NULL;
  // }
  cur_ = cur_->next;
  return;
}

}  // namespace indexing
}  // namespace recomm_engine
