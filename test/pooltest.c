#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include "pool.h"
#include <fcntl.h>

void *test_cb(void *arg)
{
	sleep(5);
	printf(arg);
	
}
void *test_cb1(void *arg)
{
	sleep(5);
	printf(arg);
	

}
void *test_cb2(void *arg)
{
	sleep(5);
	printf(arg);
	

}
void *test_cb3(void *arg)
{
	sleep(5);
	printf(arg);
	

}
int main(int argc, char *argv[])
{
	char *a = "11111111111111\n";
	char *b = "22222222222222\n";
	char *c = "33333333333333\n";
	char *d = "44444444444444\n";
	int count = 10;
	miniuv_thread_pool_t *pool = miniuv_create_threadpool(4);
	while(count--)
	{
		miniuv_add_task(pool, test_cb, a);
		miniuv_add_task(pool, test_cb1, b);
		miniuv_add_task(pool, test_cb2, c);
		miniuv_add_task(pool, test_cb3, d);
	}

	printf("add finsih............\n");
	miniuv_destory_threadpool(pool);
    printf("exit....................\n");
    return 0;
}