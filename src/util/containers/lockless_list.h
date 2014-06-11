// Coopyright 2013 SINA Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Description: Lockless list, for single thread writing
// and multi-thread reading

#ifndef _UTIL_CONTAINERS_LOCKLESS_LIST_H__
#define _UTIL_CONTAINERS_LOCKLESS_LIST_H__

#include <list>
#include <string>
#include <utility>

#include "common.h"
#include "util/base/atomic.h"
#include "glog/logging.h"
#include "gtl/stl_util-inl.h"
#include "util/concurrency/mutex.h"

typedef unsigned int uint32;

namespace util {

template<typename Type>
class ListNode {
 public:
  ListNode() : data(new Type()), next(NULL) {}
  ~ListNode() { delete data; }

  Type *data;
  const ListNode *next;

 private:
  DISALLOW_COPY_AND_ASSIGN(ListNode);
};

template<typename T>
class ListAllocator;

// A list structure that allow one write thread and multi read thread
// mutate / access the list without lock.
// The list is a fifo structure, could only be appended in front and deleted
// in back.
template<typename Type, typename Alloc = ListAllocator<Type> >
class LocklessList {
 public:
  explicit LocklessList(Alloc *allocator = NULL) : destroy_(false),
                                                   front_(NULL),
                                                   allocator_(allocator),
                                                   node_num_(0) {
    if (allocator_ == NULL) {
      VLOG(1) << "use own allocator";
      destroy_ = true;
      allocator_ = new Alloc();
    }
    front_ = allocator_->Allocate();
    front_->next = NULL;
  }
  ~LocklessList() {
    uint32 count = allocator_->Recycle(front_);
    CHECK(count == node_num_ + 1);
    AtomicPointerAssgin(&front_, reinterpret_cast<ListNode<Type>*>(NULL));
    if (destroy_)
      delete allocator_;
  }

  inline void Front(const ListNode<Type> **front) const {
    AtomicPointerAssgin(
        front, reinterpret_cast<const ListNode<Type>*>(front_));
  }

  inline ListNode<Type> *Allocate() {
    return allocator_->Allocate();
  }
	inline ListNode<Type> *Allocate(const Type& t) {
	  ListNode<Type>* node = allocator_->Allocate();
		if (NULL == node) {
			LOG(ERROR) << "Fail to allocate memory for " << sizeof(ListNode<Type>)
			           << "bytes";
		  return NULL;
		}
		node->data = new Type(t);
		return node;
	}

  inline void PushFront(ListNode<Type> *node) {
    node->next = front_->next;
    AtomicPointerAssgin(
        const_cast<ListNode<Type>**>(&front_->next), node);
    ++node_num_;
  }

  // erase all nodes after this node.
  inline void EraseAfter(const ListNode<Type> *node) {
    if (!node) return;
    const ListNode<Type> *p = node->next;
    AtomicPointerAssgin(const_cast<const ListNode<Type>**>(&node->next),
                              reinterpret_cast<const ListNode<Type>*>(NULL));
    uint32 count = allocator_->Recycle(p);
    node_num_ -= count;
  }

  inline bool IsEmpty() const {
    if (front_->next == NULL) {
      CHECK_EQ(0, node_num_);
      return true;
    }
    return false;
  }

  inline void Reset() {
    EraseAfter(front_);
    CHECK(IsEmpty());
  }

  inline uint32 node_num() const {
    return node_num_;
  }

 private:
  bool destroy_;
  ListNode<Type> *front_;
  Alloc *allocator_;
  uint32 node_num_;

  DISALLOW_COPY_AND_ASSIGN(LocklessList);
};

// Object to manage the node objects for the lockless list.
// It is not thread safe.
template<typename Type>
class ListAllocator {
 public:
  ListAllocator() : buffer_num_(0) {}
  explicit ListAllocator(int buffer_num) : buffer_num_(buffer_num) {}
  ~ListAllocator() {
    gtl::STLDeleteElements(&obj_pool_);
  }

 private:
  friend class LocklessList<Type>;
  ListNode<Type> *Allocate() {
    MutexLock l(mu_);
    if (obj_pool_.size() <= buffer_num_) {
      return new ListNode<Type>();
    }
    ListNode<Type> *node = const_cast<ListNode<Type>*>(obj_pool_.front());
    node->next = NULL;
    obj_pool_.pop_front();
    return node;
  }

  uint32 Recycle(const ListNode<Type> *node) {
    MutexLock l(mu_);
    uint32 count = 0;
    while (node) {
      obj_pool_.push_back(node);
      node = node->next;
      ++count;
    }
    return count;
  }

  uint32 buffer_num_;
  std::list<const ListNode<Type> *> obj_pool_;
	Mutex mu_;
  DISALLOW_COPY_AND_ASSIGN(ListAllocator);
};

}  // namespace util 
#endif  // _UTIL_CONTAINERS_LOCKLESS_LIST_H__
