#ifndef LIBMINIUV_POOL_H
#define LIBMINIUV_POOL_H



typedef struct miniuv_thread_task
{
    void (*cb)(void *arg);
    void *arg;
} miniuv_thread_task_t;

typedef struct miniuv_thread_pool
{
    int thread_num;
    int empty_num;
    int exit;
    pthread_mutex_t lock;
    pthread_cond_t  work_cond;
    pthread_cond_t  idle_cond;
    miniuv_thread_task_t task;
    pthread_t *thread;
} miniuv_thread_pool_t;

#endif