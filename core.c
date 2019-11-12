#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "core.h"
#include <stdlib.h>
#include "pool.h"
#include "util.h"

#define MAX_FD_NUM 65535

miniuv_container_t * miniuv_create_new_container(int type)
{
    miniuv_container_t *cont = NULL;
    struct epoll_event ev;
    int fd = -1;
    int ret = -1;
    miniuv_debug("input arg: type: %d\n", type);
    fd = epoll_create(MAX_FD_NUM);
    if(fd < 0){
        miniuv_debug("create_epoll error fd: %d\n", fd);
        return cont;
    }
    cont = malloc(sizeof(miniuv_container_t));
    if(cont == NULL)
    {
        miniuv_debug("malloc failed\n");
        return cont;
    }
    ret = pipe(cont->pipefd);
    if(ret != 0)
    {
        miniuv_debug("create pipe failed\n");
        goto failed;
    }
    cont->fd = fd;
    ev.data.fd = cont->pipefd[0];
    ev.events = EPOLLIN;
    ret = epoll_ctl(cont->fd, EPOLL_CTL_ADD, ev.data.fd, &ev);
    if(ret != 0)
    {
        miniuv_debug("epoll_ctl failed ret: %d\n", ret);
        goto failed;
    }
    cont->node = NULL;
    return cont;
failed:
    free(cont);
    cont = NULL;
    return cont;
}
int miniuv_delete_container(miniuv_container_t *cont)
{
    int ret = -1, ctrl = MINIUV_STOP_RUNNING;
    miniuv_debug("\n");
    ret = write(cont->pipefd[1], &ctrl, sizeof(ctrl));
    if(ret <= 0)
    {
        miniuv_debug("stop running failed ret: %d\n", ret);
        return FAILED;
    }
    return SUCCESS;
}
static int __delete_container(miniuv_container_t *cont)
{
    miniuv_event_node_t *node = NULL, *tmp = NULL;
    struct epoll_event ev;
    int ret = -1;
    miniuv_debug("\n");
    if(cont == NULL)
    {
        miniuv_debug("delete failed: miniuv_container_t is null\n");
        return FAILED;
    }
    // free the registed_event
    tmp = cont->node;
    for(node = cont->node; node != NULL; node = tmp->next, tmp = tmp->next)
    {
        miniuv_debug("free time\n");
        ret = epoll_ctl(cont->fd, EPOLL_CTL_DEL, node->ev.data.fd, &node->ev);
        if(ret != 0)
        {
            miniuv_debug("epoll_ctl delete fd %d failed ret: %d\n",node->ev.data.fd, ret);
        }
        free(node);
    }
    // free the container buffer
    ev.data.fd = cont->pipefd[0];
    ev.events = EPOLLIN;
    ret = epoll_ctl(cont->fd, EPOLL_CTL_DEL, ev.data.fd, &ev);
    if(ret != 0)
    {
        miniuv_debug("epoll_ctl delete fd %d failed ret: %d\n",node->ev.data.fd, ret);
    }
    close(cont->fd);
    close(cont->pipefd[0]);
    close(cont->pipefd[1]);
    free(cont);
    return SUCCESS;
}


int miniuv_register_event(miniuv_container_t *cont, int fd, int event, miniuv_event_cb_t cb)
{
    miniuv_event_node_t *node = NULL, *tmp = NULL;
    int ret;
    miniuv_debug("input arg: fd: %d, event: %d\n",fd , event);
    if(cont == NULL)
    {
        miniuv_debug("register event failed: miniuv_container_t is null\n");
        return FAILED;
    }
    if(fd < 0)
    {
        miniuv_debug("register event failed: invalid fd %d\n", fd);
        return FAILED;
    }
    if(cb == NULL)
    {
        miniuv_debug("register event failed: cb is null\n");
        return FAILED;
    }
    node = malloc(sizeof(miniuv_event_node_t));
    if(node == NULL)
    {
        miniuv_debug("malloc failed\n");
        return FAILED;
    }
    node->ev.events = event;
    node->ev.data.fd = fd;
    node->next = NULL;
    node->cb = cb;
    // add a new event to the container 
    ret = epoll_ctl(cont->fd, EPOLL_CTL_ADD, fd, &node->ev);
    if(ret != 0)
    {
        miniuv_debug("epoll_ctl failed ret: %d\n", ret);
        free(node);
        return FAILED;
    }
    if(cont->node == NULL)
    {
        // add first node in the container
        cont->node = node;
        return SUCCESS;
    }
    for(tmp = cont->node; tmp->next != NULL; tmp = tmp->next);
    tmp->next = node;

    return SUCCESS;
}

int miniuv_run(miniuv_container_t *cont)
{
    struct epoll_event ev;
    miniuv_event_node_t *node = NULL, *tmp = NULL;
    int ret = -1, ctrl = -1, len = -1;
    miniuv_debug("\n");
    if(cont == NULL)
    {
        miniuv_debug("running failed: miniuv_container_t is null\n");
        return FAILED;
    }
    while(1)
    {
        ret = epoll_wait(cont->fd, &ev, 1, -1);
        if(ret <= 0) continue;

        if(cont->pipefd[0] == ev.data.fd)
        {
            len  = read(cont->pipefd[0],&ctrl, sizeof(int));
            if(len < 0) continue;
            if(ctrl == MINIUV_STOP_RUNNING)
            {
                miniuv_debug("recv stop running signal\n");
                __delete_container(cont);
                break;
            }
        }
        miniuv_debug("get a fd: %d, event: %d\n", ev.data.fd, ev.events);
        for(node = cont->node; node != NULL; node = node->next)
        {
            miniuv_debug("seach fd: %d, event: %d\n", node->ev.data.fd, node->ev.events);
            if(node->ev.data.fd == ev.data.fd)
            {
                miniuv_debug("found event in fd: %d event: %d\n", ev.data.fd, ev.events);
                (*node->cb)(ev.data.fd, &ev);
            }
        }
    }
    return ctrl;
}