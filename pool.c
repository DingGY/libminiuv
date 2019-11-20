#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "pool.h"
#include "util.h"
static int __push_task_queue(miniuv_thread_pool_t *pool, miniuv_thread_task_t *node)
{
    miniuv_thread_task_t *tmp = NULL;

    if(node == NULL)
        return FAILED;

    pool->task_count++;

    if(pool->task_queue == NULL)
    {
        pool->task_queue = node;
        return SUCCESS;
    }  
    tmp = pool->task_queue;
    pool->task_queue = node;
    pool->task_queue->next = tmp;
    return SUCCESS;
}


static int __pop_task_queue(miniuv_thread_pool_t *pool, miniuv_thread_task_t **node)
{
    miniuv_thread_task_t *fw_tmp = NULL, *queue = pool->task_queue;
    if(queue == NULL)
        return FAILED;
    pool->task_count--;
    for(queue, fw_tmp = queue;
        queue->next != NULL;
        fw_tmp = queue, queue = queue->next);
    *node = queue;
    fw_tmp->next = NULL;
    if(fw_tmp == queue)
        pool->task_queue = NULL;

    return SUCCESS;
}

/*
static int __get_task_queue_size(miniuv_thread_pool_t *pool)
{
    int count = 0;
    miniuv_thread_task_t *tmp = NULL, *queue = pool->task_queue;
    if(queue == NULL)
        return count;

    for(count = 1, tmp = queue; tmp->next != NULL; count++, tmp = tmp->next);
    return count;
}
*/

static void * __task_cb(void *arg)
{
    miniuv_thread_task_t *task;
    miniuv_thread_pool_t *pool = (miniuv_thread_pool_t *)arg;
    while(1)
    {
        pthread_mutex_lock(&(pool->lock));
        pool->empty_num++;
        pthread_cond_broadcast(&(pool->idle_cond));
        while(pool->task_queue == NULL && !pool->exit)
        {
            //avoid spurious wakeup
            miniuv_debug("thread idle wait ......\n");
            pthread_cond_wait(&(pool->work_cond), &(pool->lock));
        }

        if(pool->exit && pool->task_queue == NULL)
        {
            miniuv_debug("exit thread %d ....\n", pool->empty_num);
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }
        pool->empty_num--;
        
        if(__pop_task_queue(pool, &task))
        {
             pthread_mutex_unlock(&(pool->lock));
             continue;
        }

        pthread_mutex_unlock(&(pool->lock));
        (*(task->cb))(task->arg);
        free(task->arg);
        free(task);
        
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
    pool->task_queue = NULL;
    pool->task_count = 0;
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
        if(ret)
        {
            miniuv_debug("create thread failed: index: %d\n", i);
        }
    }
    return pool;
failed:
    free(pool);
    return NULL;
}


int miniuv_add_task(miniuv_thread_pool_t *pool, void *(*cb)(void *), void *arg)
{
    miniuv_thread_task_t *task = NULL;
    if(pool == NULL)
    {
        miniuv_debug("thread pool is NULL\n");
        return FAILED;
    }
    task = malloc(sizeof(miniuv_thread_task_t));
    if(task == NULL)
    {
        miniuv_debug("task malloc failed\n");
        return FAILED;
    }
    task->arg = arg;
    task->cb = cb;
    task->next = NULL;
    pthread_mutex_lock(&(pool->lock));

    while (pool->task_count > pool->thread_num * 2)
    {
        miniuv_debug("not have empty thread, we neet to wait\n");
        pthread_cond_wait(&(pool->idle_cond), &(pool->lock));
    }
    if(__push_task_queue(pool, task))
    {
        pthread_mutex_unlock(&(pool->lock));
        free(task);
        free(arg);
        return FAILED;
    }
    pthread_mutex_unlock(&(pool->lock));
    pthread_cond_signal(&(pool->work_cond));
    return SUCCESS;
}

int  miniuv_destory_threadpool(miniuv_thread_pool_t *pool)
{
    int i = 0;
    miniuv_debug("destory threadpool.\n");
    if(pool == NULL)
    {
        miniuv_debug("thread pool is NULL.\n");
        return FAILED;
    }
    pthread_mutex_lock(&(pool->lock));
    
    // wait all work finished
    miniuv_debug("11111111.\n");
    while(pool->empty_num != pool->thread_num || pool->task_queue != NULL)
        pthread_cond_wait(&(pool->idle_cond), &(pool->lock));
    miniuv_debug("22222222222222222.\n");

    pool->exit = 1;
    pthread_mutex_unlock(&(pool->lock));
    pthread_cond_broadcast(&(pool->work_cond));
    // wait all thread exit
    for (i = 0; i < pool->thread_num; i++)
        pthread_join(pool->thread[i], NULL);

    miniuv_debug("all thread exited.\n");
    pthread_mutex_lock(&(pool->lock));
    pthread_mutex_destroy(&(pool->lock));
    pthread_mutex_unlock(&(pool->lock));
    pthread_cond_destroy(&(pool->work_cond));
    pthread_cond_destroy(&(pool->idle_cond));
    free(pool->thread);
    free(pool);
    pool = NULL;
    return SUCCESS;

}