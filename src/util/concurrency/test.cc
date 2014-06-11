
#include "./thread.h"
#include "glog/logging.h"
#include "gflags/gflags.h"
#include "./mutex.h"

util::RwMutex mutex_;
class MyThread : public util::Thread {
  public:
	  MyThread(int count):cnt_(count)  {
		}
		void Run(){
			for (int i = 0 ; i < cnt_; i++) {
			  LOG(INFO) << "Sleeping " << i;
				util::ReaderMutexLock lk(mutex_);
				sleep(1);
			}
		}
  private:
	  int cnt_;
};

int main(int argc, char** argv) {
	MyThread my_th(5);
	my_th.Start();
	{
		sleep(1);
		util::WriterMutexLock lk(mutex_);
		sleep(5);
	}

	// my_th.Join();
  return 0;
}
