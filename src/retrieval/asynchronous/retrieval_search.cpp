#include "retrieval_search.h"
#include <stdio.h>
#include <sys/time.h>

retrieval_search::retrieval_search(){
}

retrieval_search::~retrieval_search(){
}

int retrieval_search::open(size_t thread_num, size_t stack_size) {
        return retrieval_task::open(thread_num, stack_size);
}

int retrieval_search::deactivate()
{
        m_task_list.flush();
        join();
        return 0;
}

int retrieval_search::svc()
{
        retrieval_worker* worker;
		RetrievalHandler* retrieval_handler = new RetrievalHandler();

        while ((worker = m_task_list.get()) != NULL)
 	    {
			
			struct timeval begin_search_time;
			gettimeofday(&begin_search_time,NULL);
			
			retrieval_handler->search(worker->response, worker->request);
			
			struct timeval end_search_time;
			gettimeofday(&end_search_time,NULL);
			long cost = (end_search_time.tv_sec - begin_search_time.tv_sec)*1000000 + (end_search_time.tv_sec-begin_search_time.tv_sec);

			LOG(INFO)<<"cost = " << cost << "\n";





		 	/*

			  //  put the worker into the next stage;
			  //  callback: return the result
		    */	   

        }
        delete retrieval_handler;
		retrieval_handler = NULL;
        return 0;
}
