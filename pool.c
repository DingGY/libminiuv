#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include "pool.h"
#include "util.h"

static void __task_cb(void *pool)
{
    miniuv_thread_task_t task;
    while(1)
    {
        pthread_mutex_lock(&(pool->lock));
        pool->empty_num++;
        pthread_cond_wait(&(pool->work_cond), &(pool->lock));
        if(pool->exit)
        {
            miniuv_debug("exit thread....\n");
            pthread_exit(NULL);
        }
        memcpy(task, pool->task, sizeof(miniuv_thread_task_t));
        pool->empty_num--;
        pthread_mutex_unlock(&(pool->lock));
        (*(task.cb))(task.arg);
        pthread_cond_broadcast(&(pool->idle_cond));
    }
}

miniuv_thread_pool_t *miniuv_create_threadpool(int num)
{
    miniuv_thread_pool_t *pool;
    int i, ret;
    pool = malloc(sizeof(miniuv_thread_pool_t));
    if(pool == NULL)
    {
        miniuv_debug("failed to malloc thread pool\n");
        return pool;
    }
    pool->thread_num = num;
    pool->empty_num = 0;
    pool->thread = malloc(sizeof(pthread_t) * pool->thread_num);
    if(pool->thread == NULL)
    {
        miniuv_debug("failed to malloc thread array\n");
        goto failed;
    }
    if (pthread_mutex_init(&(pool->lock), NULL) ||
        pthread_cond_init(&(pool->work_cond), NULL) ||
        pthread_cond_init(&(pool->idle_cond), NULL))
    {
        miniuv_debug("failed to init lock or cond\n");
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->work_cond));
        pthread_cond_destroy(&(pool->idle_cond));
        goto failed;
    }

    for(i = 0; i < pool->thread_num; i++)
    {
        ret = pthread_create(&(pool->thread[i]), NULL, __task_cb, (void *)pool);
        if(!ret)
        {
            miniuv_debug("create thread failed: index: %d\n", i);
        }
    }
    return pool;
failed:
    free(pool);
    return NULL;
}

void miniuv_add_task(miniuv_thread_pool_t *pool, void *(*cb)(void *), void *arg)
{

    pthread_mutex_lock(&(pool->lock));

    while (!pool->empty_num)
    {
        miniuv_debug("not have empty thread, we neet to wait\n");
        pthread_cond_wait(&(pool->idle_cond), &(pool->lock));
    }
    pool->task.cb = cb;
    pool->task.arg = arg;
    pthread_mutex_unlock(&(pool->lock));
    pthread_cond_signal(&(pool->work_cond));
    return ;
}