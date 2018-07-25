/* ************************************************************************
 *       Filename:  event_tcp_server.c
 *    Description:  
 *        Version:  1.0
 *        Created:  2018年06月21日 11时38分22秒
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/


#include <stdio.h>
#include <event.h>
#include "wrap.h"
struct event *ev;
void cfdcb( int fd,short events,void* arg)
{
	struct event_base *base = (struct event_base *)arg;
//	while(1) 不能有循环
	{
		char buf[1024]="";
		int n = Read(fd,buf,sizeof(buf));
		if(n <= 0)
		{
			close(fd);
			event_del(ev);//下树
		}
		else
		{
			printf("buf=%s\n",buf);
			Write(fd,buf,n);
		}
	}
	
}
void lfdcb( int fd,short events,void* arg)
{
	struct event_base *base = (struct event_base *)arg;
	int cfd = Accept(fd,NULL,NULL);//提取链接

	ev = event_new(base,cfd,EV_READ|EV_PERSIST,cfdcb,base);
	event_add(ev,NULL);

}
int main(int argc, char *argv[])
{
	//创建套接字
	int lfd = tcp4bind(8000,NULL);
	//绑定
	//监听
	listen(lfd,128);
	//创建根节点
	struct event_base *base = event_base_new();
	//初始化节点
	struct event *ev = event_new(base,lfd,EV_READ|EV_PERSIST,lfdcb,base);
	//上树
	event_add(ev,NULL);
	//循环监听
	event_base_dispatch(base);//循环阻塞
	//扫尾
	event_base_free(base);
	return 0;
}




