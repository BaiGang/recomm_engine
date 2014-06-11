// Copyright 2013. Sina .com. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Description: Producer-Consumer Queue with timeout.

#ifndef _UTIL_CONTAINERS_PRODUCER_CONSUMER_QUEUE_H_ 
#define _UTIL_CONTAINERS_PRODUCER_CONSUMER_QUEUE_H_ 
#include <deque>
#include "glog/logging.h"
#include "util/base/timer.h"

using std::deque;  // NOLINT
namespace util {
template<typename Item>
// Inner item mainted by producer-consumer queue
class InnerItem {
 public:
  InnerItem(const Item& _item, const int64_t& _timestamp,
            const int64_t& _timeout): item_(_item),
    timestamp_(_timestamp),
    timeout_(_timeout) {
  }
  Item item_;
  // timestamp
  int64_t timestamp_;
  int64_t timeout_;
};

template<typename Item>
class ProducerConsumerQueue {
 public:
  // Constructor
  // @capacity: The max number of items can be pushed into PCQ
  // @default_timeout: The timeout in us for each item to reside in PCQ
  //  if the time span from being pushed into PCQ to gettting out PCQ
  //  exceeds timeout, the item will be simply discarded.
  ProducerConsumerQueue(const int& _capacity,
                        const int64_t& _default_timeout = 3000)
      :capacity_(_capacity), default_timeout_(_default_timeout) {
    CHECK(capacity_ > 0 && capacity_ < INT_MAX)
        << "Invalid capacity";
    // Init mutex
    pthread_mutex_init(&mutex_t_, NULL);
    // Init conditional variables
    pthread_cond_init(&put_cond_t_, NULL);
    pthread_cond_init(&get_cond_t_, NULL);
  }
  // Destructor
  ~ProducerConsumerQueue() {
    que_.clear();
    pthread_mutex_destroy(&mutex_t_);
    pthread_cond_destroy(&put_cond_t_);
    pthread_cond_destroy(&get_cond_t_);
  }
  bool ForcePut(const Item& item, const int64_t& _timeout =
                TIMEOUT_USE_DEFAULT) {
    // lock
    pthread_mutex_lock(&mutex_t_);
    CHECK(ForcePutInternal(item, _timeout)) << "Fail to force put";
    pthread_mutex_unlock(&mutex_t_);
    return true;
  }
  // Put, will block if inner queue is full
  void Put(const Item& item,
           const int64_t& _timeout = ProducerConsumerQueue::TIMEOUT_USE_DEFAULT) {
    // lock
    pthread_mutex_lock(&mutex_t_);
    // wait till there is no available slot
    while (que_.size() >= static_cast<unsigned int>(capacity_)) {
      VLOG(3) << "Waiting for queue slot..";
      pthread_cond_wait(&put_cond_t_, &mutex_t_);
      VLOG(3) << "Wake up";
    }
    CHECK(TryPutInternal(item, _timeout));
    // unlock
    pthread_mutex_unlock(&mutex_t_);
  }
  // Try to put, non-blocking
  bool TryPut(const Item& item,
              const int64_t& _timeout =
              ProducerConsumerQueue::TIMEOUT_USE_DEFAULT) {
    bool ret = false;
    // lock
    pthread_mutex_lock(&mutex_t_);
    // Try put
    ret =  TryPutInternal(item, _timeout);
    // unlock
    pthread_mutex_unlock(&mutex_t_);
    return ret;
  }
  // Get, will block if inner queue is empty
  void Get(Item* item) {
    pthread_mutex_lock(&mutex_t_);
    while (!TryGetInternal(item)) {
      pthread_cond_wait(&get_cond_t_, &mutex_t_);
    }
    pthread_mutex_unlock(&mutex_t_);
  }
  // Try to get an element, non-blocking
  bool TryGet(Item* item) {
    // lock
    pthread_mutex_lock(&mutex_t_);
    bool ret = TryGetInternal(item);
    // unlock
    pthread_mutex_unlock(&mutex_t_);
    return ret;
  }
  // Provide the estimated size, without locking the container
  size_t RoughSize() {
    return que_.size();
  }
  // Provde the accurate size, lock the container,
  // do not use this method frequently
  size_t PreciseSize() {
    size_t size = 0;
    pthread_mutex_lock(&mutex_t_);
    size = que_.size();
    pthread_mutex_unlock(&mutex_t_);
    return size;
  }
  static const int TIMEOUT_NONE = 0;
  static const int TIMEOUT_USE_DEFAULT = -1;
 private:
  bool ForcePutInternal(const Item& item,
                        const int64_t& _timeout =
                        ProducerConsumerQueue::TIMEOUT_USE_DEFAULT) {
    int64_t timeout = _timeout;
    if (timeout == TIMEOUT_USE_DEFAULT) {
      timeout = default_timeout_;
    }
    if (que_.size() >= capacity_) {
      que_.pop_front();
    }
    InnerItem<Item> itm(item, Timer::CurrentTimeInUs(), timeout);
    que_.push_back(itm);
    // Signal the condition variable
    // to notify consumers
    pthread_cond_signal(&get_cond_t_);
    return true;
  }
  bool TryPutInternal(const Item& item, const int64_t& _timeout =
                      ProducerConsumerQueue::TIMEOUT_USE_DEFAULT) {
    bool ret = false;
    if (que_.size() >= static_cast<unsigned int>(capacity_)) {
      ret = false;
    } else {
      int64_t timeout = _timeout;
      if (_timeout == TIMEOUT_USE_DEFAULT) {
        timeout = default_timeout_;
      }
      InnerItem<Item> itm(item, Timer::CurrentTimeInUs(), timeout);
      que_.push_back(itm);
      ret = true;
    }
    // Signal the conditional variable
    pthread_cond_signal(&get_cond_t_);
    return ret;
  }
  bool TryGetInternal(Item* item) {
    bool ret = false;
    while (!que_.empty()) {
      InnerItem<Item> itm = que_.front();
      que_.pop_front();
      if (itm.timeout_ == TIMEOUT_NONE ||
          (Timer::CurrentTimeInUs() <=
           itm.timestamp_ + itm.timeout_)) {
        *item = itm.item_;
        ret = true;
        VLOG(3) << "Normal job" << Timer::CurrentTimeInUs() << ":"
            << itm.timestamp_ << ":" << itm.timeout_;
        break;
      } else {
        VLOG(3) << "Time out job:" << Timer::CurrentTimeInUs() << ":"
            << itm.timestamp_ << ":" << itm.timeout_;
        continue;
      }
    }
    pthread_cond_signal(&put_cond_t_);
    return ret;
  }
  // Mutex
  pthread_mutex_t mutex_t_;
  // Conditional Varialbles
  pthread_cond_t put_cond_t_;
  pthread_cond_t get_cond_t_;
  // Capacity
  int capacity_;
  // inner queue
  deque<InnerItem<Item> > que_;
  // Time out in us
  int64_t default_timeout_;
};
};  // namespace util

#endif  // _UTIL_CONTAINERS_PRODUCER_CONSUMER_QUEUE_H_ 
