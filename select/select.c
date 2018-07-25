#include <stdio.h>
#include <sys/select.h>
#include "wrap.h"
int main(int argc, char const *argv[])
{
	
	unsigned short port = 8000;
	if(argc >= 2)
		port = atoi(argv[1]);
	int lfd = tcp4bind(port,NULL);//创建绑定
	listen(lfd,128);//监听
	fd_set rset;
	fd_set oldset;
	FD_ZERO(&oldset);
	FD_ZERO(&rset);
	FD_SET(lfd,&oldset);

	int maxfd = lfd;
	int ret=0;
	while(1)
	{
		rset = oldset;
		ret = select(maxfd+1 ,&rset,NULL,NULL,NULL);
		if(ret <= 0)
		{
			continue;
		}
		if(ret > 0)
		{
			//如果是lfd变化了
			if(FD_ISSET(lfd,&rset))
			{
				struct sockaddr_in cliaddr;
				socklen_t len = sizeof(cliaddr);
				char buf_ip[16]="";
				int cfd = Accept(lfd,(struct sockaddr *)&cliaddr,&len);
				printf("client ip=%s  port=%d\n", 
				inet_ntop(AF_INET,&cliaddr.sin_addr.s_addr,buf_ip,sizeof(buf_ip) ),
				ntohs(cliaddr.sin_port));
				FD_SET(cfd,&oldset);
				if(cfd > maxfd)//如果提取的cfd比maxfd大，则将cfd赋值给maxfd
				 maxfd = cfd;
				if(--ret == 0)//如果只有lfd变化，返回值ret==1，
					continue;




			}
			//普通的cfd变化
			for(int i=lfd+1;i<=maxfd;i++)
			{
				if(FD_ISSET(i,&rset))//如果cfd有变化，收到客户端的数据
				{	
						char buf[1024]="";
						int n = Read(i,buf,sizeof(buf));
						if(n <= 0 )
						{
							close(i);
							FD_CLR(i,&oldset);
							printf("erron or client close\n");

						}
						else
						{
							printf("%s\n",buf );
							Write(i,buf,n);

						}

				}

			}

		}


	}


	return 0;
}