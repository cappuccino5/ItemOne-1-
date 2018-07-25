/*
  This exmple program provides a trivial server program that listens for TCP
  connections on port 9995.  When they arrive, it writes a short message to
  each client connection, and closes each connection once it is flushed.

  Where possible, it exits cleanly in response to a SIGINT (ctrl-c).
*/


#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

static const char MESSAGE[] = "Hello, World!\n";

static const int PORT = 9995;

static void listener_cb(struct evconnlistener *, evutil_socket_t,
    struct sockaddr *, int socklen, void *);
static void conn_writecb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);
static void signal_cb(evutil_socket_t, short, void *);
static void rcb(struct bufferevent *bev, void *user_data);

int
main(int argc, char **argv)
{
	struct event_base *base;//根节点地址
	struct evconnlistener *listener;//连接侦听器的地址
	struct event *signal_event;//event事件节点的地址

	struct sockaddr_in sin;
#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
#endif

	base = event_base_new();//创建base根节点
	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}
	//初始化绑定的结构体
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	//创建连接监听器  socket  bind  listen  accept
	//
	//有新的连接到来，自动回调listener_cb 函数
	listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
	    LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
	    (struct sockaddr*)&sin,
	    sizeof(sin));

	if (!listener) {
		fprintf(stderr, "Could not create a listener!\n");
		return 1;
	}

	signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
	//event_new(base,SIGINT,signal_cb,base);

	if (!signal_event || event_add(signal_event, NULL)<0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}

	event_base_dispatch(base);//循环监听

	evconnlistener_free(listener);//释放链接侦听器 并且自动关闭lfd
	event_free(signal_event);
	event_base_free(base);

	printf("done\n");
	return 0;
}

static void
listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
    struct sockaddr *sa, int socklen, void *user_data)
{
	struct event_base *base = user_data;
	struct bufferevent *bev;

	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);//创建一个新的节点，返回新的节点的地址
	if (!bev) {//判断是否失败
		fprintf(stderr, "Error constructing bufferevent!");
		event_base_loopbreak(base);
		return;
	}
	bufferevent_setcb(bev, rcb, conn_writecb, conn_eventcb, NULL);//设置3个回调
	bufferevent_enable(bev, EV_WRITE | EV_READ);//让写事件使能 监听写事件
//	bufferevent_disable(bev, EV_READ);//让读事件非使能  不监听读事件

//	bufferevent_write(bev, MESSAGE, strlen(MESSAGE));//把数据写入到bufferevent自带的缓冲区
}

static void rcb(struct bufferevent *bev, void *user_data)
{
	char buf[1024]="";
	int n	= bufferevent_read(bev,buf,sizeof(buf));
	printf("%s\n",buf);
	bufferevent_write(bev,buf,n);

}




static void
conn_writecb(struct bufferevent *bev, void *user_data)
{
	printf("#############wokao#####\n");
	struct evbuffer *output = bufferevent_get_output(bev);//获取bufferevent缓冲区数据
	if (evbuffer_get_length(output) == 0) {//如果bufferevent缓冲区数据为0
		printf("flushed answer\n");
	//	bufferevent_free(bev);//释放节点 关闭文件描述符cfd
	}
}

static void
conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
	if (events & BEV_EVENT_EOF) {
		printf("Connection closed.\n");
	} else if (events & BEV_EVENT_ERROR) {
		printf("Got an error on the connection: %s\n",
		    strerror(errno));/*XXX win32*/
	}
	/* None of the other events can happen here, since we haven't enabled
	 * timeouts */
	bufferevent_free(bev);
}

static void
signal_cb(evutil_socket_t sig, short events, void *user_data)
{
	struct event_base *base = user_data;
	struct timeval delay = { 2, 0 };

	printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

	event_base_loopexit(base, &delay);
}
