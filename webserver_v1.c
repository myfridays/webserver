//#define _DEFAULT_SOURCE
//#include <unistd.h>
//#include <sys/epoll.h>
//#include <fcntl.h>
//#include <sys/stat.h>
//#include <string.h>
//#include <signal.h>
//#include <dirent.h>
//
//#include "pub.h"
//#include "wrap.h"
//
//int http_request(int cfd, int epfd);
//
//int main()
//{
//	// 当web服务器给浏览器发送数据的时候，浏览器关闭连接
//	// 则web服务器就会收到SIGPIPE信号
//	struct sigaction act;
//	act.sa_handler = SIG_IGN;
//	sigemptyset(&act.sa_mask);
//	act.sa_flags = 0;
//	sigaction(SIGPIPE, &act, NULL);
//
//	// 改变当前进程的工作目录
//	char path[255] = { 0 };
//	sprintf(path, "%s/%s", getenv("HOME"), "projects/webserver");
//	chdir(path);
// 
//	// 创建socket--端口复用--bind
//	int lfd = tcp4bind(9898, NULL);
//
//	// 监听端口
//	Listen(lfd, 128);
//
//	// 创建epoll树
//	int epfd = epoll_create(1024);
//	if (epfd < 0)
//	{
//		perror("epoll_create error");
//		return -1;
//	}
//
//	// 将监听描述符上树
//	struct epoll_event ev;
//	ev.data.fd = lfd;
//	ev.events = EPOLLIN;
//	epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
//
//	int nready;
//	int sockfd;
//	int cfd;
//	struct epoll_event events[1024];
//	int i;
//	printf("111\n");
//	while (1)
//	{
//		// 等待事件发生
//		nready = epoll_wait(epfd, events, 1024, -1);
//		if (nready < 0)
//		{
//			if (nready == EINTR)
//			{
//				continue;
//			}
//			break;
//		}
//		printf("222\n");
//		// 接收客户端的请求
//		for (i = 0; i < nready; i++)
//		{
//			sockfd = events[i].data.fd;
//			if (sockfd == lfd)
//			{
//				// 有客户端连接请求
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
//				http_request(sockfd, epfd);
//			}
//		}
//	}
//}
//
//// 服务器将响应返回给客户端
//int send_hander(int cfd, char* code, char* msg, char* fileType, int len)
//{
//	printf("send_hander0\n");
//
//	char buf[1024];
//	sprintf(buf, "HTTP/1.1 %s %s\r\n", code, msg);
//	sprintf(buf+strlen(buf), "Content-Type:%s\r\n", fileType);
//	if (len)
//	{
//		sprintf(buf + strlen(buf), "Content-Length:%d\r\n", len);
//	}
//	printf("send_hander1\n");
//
//	strcat(buf, "\r\n");
//	Write(cfd, buf, strlen(buf));
//
//	printf("send_hander2\n");
//	return 0;
//}
//
//// 服务器将响应体返回给客户端
//int send_file(int cfd, char* fileName)
//{
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
//		printf("---\n");
//		memset(buf, 0x00, sizeof(buf));
//		n = read(fd, buf, sizeof(buf));
//		if (n <= 0)
//		{
//			break;
//		}
//		else
//		{
//			Write(cfd, buf, n);
//		}
//	}
//}
//
//// 服务器解析客户端的请求
//int http_request(int cfd, int epfd)
//{
//	printf("333\n");
//	char buf[1024];
//	int n;
//	memset(buf, 0x00, sizeof(buf));
//
//	n = Readline(cfd, buf, sizeof(buf));
//	if (n <= 0)
//	{
//		// 关闭连接
//		close(cfd);
//
//		// 将文件描述符下树
//		epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
//		return -1;
//	}
//
//	printf("buf==[%s]\n", buf);
//	
//	char reqType[16] = { 0 };
//	char fileName[255] = { 0 };
//	char protocal[16] = { 0 };
//	sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", reqType, fileName, protocal);
//
//	// 打印日志
//	printf("reqType==[%s]\n", reqType);
//	printf("fileName==[%s]\n", fileName);
//	printf("protocal==[%s]\n", protocal);
//	
//	char* pFile = fileName;
//	if (strlen(fileName) <= 1)
//	{
//		strcpy(pFile, "./");
//	}
//	else
//	{
//		pFile = fileName + 1;
//	}
//	strdecode(pFile, pFile);
//	printf("pFile==[%s]\n", pFile);
//
//	// 将剩余字符取完,避免产生粘包
//	while ((n = Readline(cfd, buf, sizeof(buf))) > 0);
//
//	printf("read over\n");
//
//	struct stat st;
//	// 判断是否有该文件
//	if (stat(pFile, &st) < 0)
//	{
//		printf("file is not exist\n");
//
//		// 发送头部信息
//		send_hander(cfd, "404", "NOT Found", get_mime_type(".html"), 0);
//
//		// 发送文件内容
//		send_file(cfd, "error.html");
//	}
//	else
//	{
//		printf("file is exist\n");
//		if (S_ISREG(st.st_mode))
//		{
//			// 发送头部信息
//			send_hander(cfd, "200", "OK", get_mime_type(pFile), st.st_size);
//
//			// 发送头部信息
//			send_file(cfd, pFile);
//		}
//		else if (S_ISDIR(st.st_mode))
//		{
//			printf("目录文件\n");
//
//			// 发送头部信息
//			send_hander(cfd, "200", "OK", get_mime_type(".html"), 0);
//
//			// 发送html文件头部
//			send_file(cfd, "html/dir_header.html");
//
//			struct dirent **namelist;
//			int num;
//			char buffer[1024];
//
//			num = scandir(pFile, &namelist, NULL, alphasort);
//			if (num < 0)
//			{
//				perror("scandir");
//				close(cfd);
//				epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
//				return -1;
//			}
//			else
//			{
//				while (num--)
//				{
//					printf("%s\n", namelist[num]->d_name);
//					memset(buffer, 0x00, sizeof(buffer));
//					if (namelist[num]->d_type == DT_DIR)
//					{
//						sprintf(buffer, "<li><a href=%s/>%s</a></li>", namelist[num]->d_name, namelist[num]->d_name);
//					}
//					else
//					{
//						sprintf(buffer, "<li><a href=%s>%s</a></li>", namelist[num]->d_name, namelist[num]->d_name);
//					}
//					free(namelist[num]);
//					Write(cfd, buffer, strlen(buffer));
//				}
//				free(namelist);
//			}
//			// 发送html文件尾部
//			send_file(cfd, "html/dir_tail.html");
//		}
//	}
//
//
//}
