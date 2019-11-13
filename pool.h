#ifndef LIBMINIUV_POOL_H
#define LIBMINIUV_POOL_H



typedef struct miniuv_thread_task
{
    void *(*cb)(void *arg);
    void *arg;
    struct miniuv_thread_task *next;
} miniuv_thread_task_t;

typedef struct miniuv_thread_pool
{
    int thread_num;
    int empty_num;
    int exit;
    int task_count;
    pthread_mutex_t lock;
    pthread_cond_t  work_cond;
    pthread_cond_t  idle_cond;
    miniuv_thread_task_t *task_queue;
    pthread_t *thread;
} miniuv_thread_pool_t;
int  miniuv_destory_threadpool(miniuv_thread_pool_t *pool);
int miniuv_add_task(miniuv_thread_pool_t *pool, void *(*cb)(void *), void *arg);
miniuv_thread_pool_t *miniuv_create_threadpool(int num);

#endif