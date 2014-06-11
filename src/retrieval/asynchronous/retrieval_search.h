#ifndef _RETRIEVAL_SEARCH_H_
#define _RETRIEVAL_SEARCH_H_

#include <string>
#include "retrieval_task_base.h"
#include "retrieval_handler.h"

/*
	a wraper to retrieval_handler
   	asynchronous version

*/

class retrieval_search: public retrieval_task_base
{
public:
        retrieval_search();
        virtual ~retrieval_search();

        virtual int open(size_t thread_num, size_t stack_size, size_t PackagesInOneBlock);
        virtual int deactivate();
        virtual int svc();

};

#endif /* _RETRIEVAL_SEARCH_H_ */

