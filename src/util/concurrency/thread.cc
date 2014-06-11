// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Description: OO Thread interface

#include "./thread.h"

namespace util {
Thread::Thread() {
}

Thread::~Thread() {
	if (thread_.get()) {
		thread_->join();
	}
}

void Thread::Start() {
  thread_.reset(new boost::thread(&Thread::Run, this));
}

void Thread::Join() {
  thread_->join();
}

}  // namespace util
