#include "glog/logging.h"
#include "./lockless_list.h"
#include "./producer_consumer_queue.h"

using namespace util;

void TestLocklessList() {
  LocklessList<int> my_list;
	for (int i = 10; i > 0; i--) {
	  // ListNode<int>* new_node = my_list.Allocate();
		ListNode<int>* new_node = my_list.Allocate(i);
		new_node->data = new int(i);
	  my_list.PushFront(new_node);
	}
	const ListNode<int>* cur;

	// iterate
	LOG(INFO) << "original list";
	my_list.Front(&cur);
	while (cur != NULL) {
	  LOG(INFO) << *(cur->data);
	  cur = cur->next;
	}

	LOG(INFO) << "Finding anchor";
	my_list.Front(&cur);
	while (cur != NULL && (*cur->data) != 5) {
	  LOG(INFO) << *(cur->data);
	  cur = cur->next;
	}

	my_list.EraseAfter(cur);

	LOG(INFO) << "After erasing";
	my_list.Front(&cur);
	while (cur != NULL) {
	  LOG(INFO) << *(cur->data);
	  cur = cur->next;
	}
}
void TestPCQ() {
  ProducerConsumerQueue<int> pcq_(1000, 0);
	pcq_.Put(1);
	pcq_.TryPut(2);
	int v;
	pcq_.Get(&v);
	LOG(INFO) << v;
	LOG(INFO) << pcq_.TryGet(&v);
	LOG(INFO) << v;
}
int main(int argc, char** argv) {
  TestPCQ();
};
