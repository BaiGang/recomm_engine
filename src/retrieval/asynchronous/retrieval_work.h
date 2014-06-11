#ifndef _RETRIEVAL_WORKER_H_
#define _RETRIEVAL_WORKER_H_

#include <pthread.h>
#include <wait_list.hpp>

/*
   goes through the whole pipeline;
*/


class retrieval_worker {
public:
        retrieval_worker() {
        }
        ~retrieval_worker() {
		}

public:
		void*  request;
		void*  response;

        linked_list_node_t task_list_node;
};

#endif /* _RETRIEVAL_WORKER_H_ */

