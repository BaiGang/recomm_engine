#ifndef RETRIEVAL_TASK_HPP
#define RETRIEVAL_TASK_HPP

#include <pthread.h>
#include "retrieval_worker.h"

class retrieval_worker;

/*
	A Base class : multi thread
*/

class retrieval_task_base {
public:
        retrieval_task_base();
        virtual ~retrieval_task_base();

        virtual int open(size_t thread_num, size_t stack_size) = 0;
        virtual int activate();
        virtual int deactivate() = 0;
        virtual int svc() = 0;
        virtual int join();

        virtual int put(retrieval_worker &worker);

private:
        static void * run_svc(void * arg);

protected:
        pthread_t * m_thread;
        size_t m_thread_num;
        pthread_barrier_t m_barrier;
        wait_list_t<retrieval_worker, &retrieval_worker::task_list_node> m_task_list;  	// msg queue
};

#endif

