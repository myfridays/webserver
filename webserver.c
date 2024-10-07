//#include <unistd.h>
//#include <sys/epoll.h>
//#include <fcntl.h>
//#include <sys/stat.h>
//#include <string.h>
//#include <signal.h>
//#include <dirent.h>
//
//#include "wrap.h"
//#include "pub.h"
//
//int http_request(int cfd);
//
//int main()
//{
//	// 改变当前进程的工作目录
//	char path[255] = { 0 };
//	sprintf(path, "%s/%s", getenv("HOME"), "projects/webserver/bin/x64/Debug");
//	chdir(path);
//
//	// 创建socket--设置端口复用--bind
//	int lfd = tcp4bind(9999, NULL);
//
//	// 设置监听
//	Listen(lfd, 128);
//
//	// 创建epoll树
//	int epfd = epoll_create(1024);
//	if (epfd < 0)
//	{
//		perror("epoll_create error");
//		close(lfd);
//		return -1;
//	}
//
//	// 将监听描述符lfd上树
//	struct epoll_event ev;
//	ev.data.fd = lfd;
//	ev.events = EPOLLIN;
//	epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
//
//	int i;
//	int sockfd, cfd;
//	int nready;	
//	struct epoll_event events[1024];
//	while (1)
//	{
//		// 等待事件发生
//		printf("nready epoll_wait");
//		nready = epoll_wait(epfd, events, 1024, -1);
//		if (nready < 0)
//		{
//			if (errno == EINTR)
//			{
//				continue;
//			}
//			break;
//		}
//
//		for (i = 0; i < nready; i++)
//		{
//			sockfd = events[i].data.fd;
//			// 有客户端连接请求
//			if (sockfd == lfd)
//			{
//				cfd = Accept(lfd, NULL, NULL);
//
//				// 设置cfd为非阻塞
//				int flag = fcntl(cfd, F_GETFL);
//				flag |= O_NONBLOCK;
//				fcntl(cfd, F_SETFL, flag);
//
//				// cfd上树
//				ev.data.fd = cfd;
//				ev.events = EPOLLIN;
//				epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
//			}
//			else
//			{
//				// 客户端有数据发来
//				http_request(cfd);
//			}
//			
//		}
//		
//	}
//
//	//close(lfd);
//	//printf("hello world\n");
//	//return 0;
//}
//
//int send_hander(int cfd, char * code, char * msg, char *fileType, int len)
//{
//	char buf[1024] = { 0 };
//	sprintf(buf, "HTTP/1.1 %s %s\r\n", code, msg);
//	sprintf(buf + strlen(buf), "Content-Type:%s\r\n", fileType);
//	if (len > 0)
//	{
//		sprintf(buf + strlen(buf), "Content-Length:%d\r\n", len);
//	}
//	strcat(buf, "\r\n");
//	Write(cfd, buf, strlen(buf));
//
//	return 0;
//}
//int send_file(int cfd, char *fileName)
//{
//	// 打开文件
//	int fd = open(fileName, O_RDONLY);
//	if (fd < 0)
//	{
//		perror("open error");
//		return -1;
//	}
//
//	char buf[1024];
//	int n;
//	while (1)
//	{
//		memset(buf, 0x00, sizeof(buf));
//		n = read(fd, buf, sizeof(buf));
//		if (n <= 0)
//		{
//			break;
//		}
//		else
//		{
//			write(cfd, buf, n);
//		}
//	}
//}
//int http_request(int cfd)
//{
//	int n;
//	char buf[1024];
//	// 读取请求行数据， 分析出要请求的资源文件名
//	memset(buf, 0x00, sizeof(buf));
//	Readline(cfd, buf, sizeof(buf));
//	printf("buf string\n");
//	printf("buf==[%s]\n", buf);
//
//	char reqType[16] = { 0 };
//	char fileName[255] = { 0 };
//	char protocal[16] = { 0 };
//
//	sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", reqType, fileName, protocal);
//
//	// 编写日志
//	printf("[%s]\n", reqType);
//	printf("[%s]\n", fileName);
//	printf("[%s]\n", protocal);
//	printf("------------\n");
//
//	char* pFile = fileName + 1;
//	printf("[%s]\n", pFile);
//
//	// 循环读取完剩余的数据
//	while ((n = Readline(cfd, buf, sizeof(buf))) > 0);
//
//	// 判断文件是否存在
//	struct stat st;
//	// 若文件不存在
//	if (stat(pFile, &st) < 0)
//	{
//		printf("file not exist\n");
//		
//		// 发送头部信息
//		send_hander(cfd, "404", "NOT FOUND", get_mime_type(".html"), 0);
//
//		// 发送文件内容
//		send_file(cfd, "error.html");
//		// 组织应答信息：http响应消息+错误页内容
//	}
//	else // 若文件存在
//	{
//		// 判断文件类型
//		if (S_ISREG(st.st_mode))	// 普通文件
//		{
//			// 发送头部信息
//			send_hander(cfd, "200", "OK", get_mime_type(pFile), st.st_size);
//
//			// 发送文件内容
//			send_file(cfd, pFile);
//		}
//		else if (S_ISDIR(st.st_mode)) // 目录文件
//		{
//
//		}
//	}
//}