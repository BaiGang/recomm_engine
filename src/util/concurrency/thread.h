// Copyright 2013 Jike Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Description: OO Thread interface

#ifndef _UTIL_CONCURRENCY_THREAD_H_
#define _UTIL_CONCURRENCY_THREAD_H_

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

namespace util {
class Thread {
  public:
		Thread();
		virtual ~Thread();
	  virtual void Run() = 0;
		void Start();
		void Join();
	private:
	  boost::shared_ptr<boost::thread> thread_;
};
}  // namespace util
#endif // _UTIL_CONCURRENCY_THREAD_H_
