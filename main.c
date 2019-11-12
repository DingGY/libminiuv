#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include "core.h"
#include <fcntl.h>
    miniuv_container_t *cont = NULL;

void test_cb(int fd,void *arg)
{
    char buf[1024] = {0};
    printf("test_cb fd:%d, event: %d\n", fd, ((struct epoll_event *)(arg))->events);
    while(recv(fd,buf,1024,0) > 0)
    {
        printf("recv: %s\n", buf);
    }
    //close(cont->fd);
    miniuv_delete_container(cont);

    //exit(0);
    //close(fd);
}
void test_cb1(int fd,void *arg)
{
    struct sockaddr_in remote_addr; //客户端网络地址结构体
	int sin_size;

    int client_sockfd;
    printf("accept fd:%d, event: %d\n", fd, ((struct epoll_event *)(arg))->events);
    sin_size=sizeof(struct sockaddr_in);
    if((client_sockfd=accept(fd,(struct sockaddr *)&remote_addr,&sin_size))<0)
	{
		perror("accept");
		return;
	}
        int flags = fcntl(client_sockfd, F_GETFL, 0);
    fcntl(client_sockfd, F_SETFL, flags | O_NONBLOCK);
    miniuv_register_event(cont, client_sockfd, EPOLLIN|EPOLLET, test_cb);
    //miniuv_delete_container(cont);

}
int main(int argc, char *argv[])
{
	int server_sockfd;//服务器端套接字
	int client_sockfd;//客户端套接字
	int len;
	struct sockaddr_in my_addr;   //服务器网络地址结构体

	char buf[BUFSIZ];  //数据传送的缓冲区
	memset(&my_addr,0,sizeof(my_addr)); //数据初始化--清零
	my_addr.sin_family=AF_INET; //设置为IP通信
	my_addr.sin_addr.s_addr=INADDR_ANY;//服务器IP地址--允许连接到所有本地地址上
	my_addr.sin_port=htons(9091); //服务器端口号
	cont = miniuv_create_new_container(0);
	/*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
	if((server_sockfd=socket(PF_INET,SOCK_STREAM,0))<0)
	{  
		perror("socket");
		return 1;
	}
    
    int flags = fcntl(server_sockfd, F_GETFL, 0);
    fcntl(server_sockfd, F_SETFL, flags | O_NONBLOCK);
        /*将套接字绑定到服务器的网络地址上*/
	if (bind(server_sockfd,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))<0)
	{
		perror("bind");
		return 1;
	}
	
	/*监听连接请求--监听队列长度为5*/
	listen(server_sockfd,5);
	
	
	
	/*等待客户端连接请求到达*/
    miniuv_register_event(cont, server_sockfd, EPOLLIN|EPOLLET, test_cb1);
    

    miniuv_run(cont);
	close(server_sockfd);
    printf("exit....................\n");
    return 0;
}