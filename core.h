#ifndef LIBMINIUV_CORE_H
#define LIBMINIUV_CORE_H

typedef void (*miniuv_event_cb_t)(int fd,void *arg) ;
typedef struct miniuv_event_node
{
    struct epoll_event ev;
    miniuv_event_cb_t cb;
    struct miniuv_event_node *next;
} miniuv_event_node_t;

typedef struct miniuv_container
{
    int fd;
    int pipefd[2];
    miniuv_thread_pool_t *thread_pool; 
    miniuv_event_node_t *node;
} miniuv_container_t;



miniuv_container_t * miniuv_create_new_container(int type);
int miniuv_delete_container(miniuv_container_t *cont);
int miniuv_register_event(miniuv_container_t *cont, int fd, int event, miniuv_event_cb_t cb);
int miniuv_run(miniuv_container_t *cont);

#endif