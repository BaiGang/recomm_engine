// Copyright 2013 Jike Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Description: Mutexes for synchronization

#ifndef _UTIL_CONCURRENCY_MUTEX_H_
#define _UTIL_CONCURRENCY_MUTEX_H_

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/shared_lock_guard.hpp>

namespace util {
  typedef boost::mutex Mutex;
	typedef boost::shared_mutex RwMutex;
	typedef boost::unique_lock<Mutex> MutexLock;
	typedef boost::shared_lock<RwMutex> ReaderMutexLock;
	typedef boost::unique_lock<RwMutex> WriterMutexLock;
}

#endif  // _UTIL_CONCURRENCY_MUTEX_H_
