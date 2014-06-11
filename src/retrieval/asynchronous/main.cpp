#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "retrieval_search.hpp"

void sigterm_handler(int signo) {}
void sigint_handler(int signo) {}

int main(int argc, char ** argv)
{
        close(STDIN_FILENO);
        signal(SIGPIPE, SIG_IGN);
        signal(SIGTERM, &sigterm_handler);
        signal(SIGINT, &sigint_handler);

        {
				retrieval_search task_search;


				if (task_search.open())
				{
					LOG(ERROR) << "task_search open failed\n";
				}
				task_search.activate();
				pause();
				task_search.deactivate();
        }

}

