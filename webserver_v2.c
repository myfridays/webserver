#define _DEFAULT_SOURCE
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>

#include "pub.h"
#include "wrap.h"

int main()
{
	// 服务器发送的数据的时候，当客户端关闭连接
	// 服务器产生SIGPIPE信号
	struct sigaction act;
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGPIPE, &act, NULL);

	// 改变当前的工作目录
	char path[1024];
	sprintf(path, "%s/%s", getenv("HOME"), "projects/webserver");
	chdir(path);
	printf("work__path:%s\n", path);
	// 创建socket--端口复用--bind
	int lfd = tcp4bind(9999, NULL);

	// 监听
	Listen(lfd, 128);

	// 创建epoll树
	int epfd = epoll_create(1024);
	if (epfd < 0)
	{
		perror("epoll_create error");
		return -1;
	}

	// 监听描述符上树
	struct epoll_event ev;
	ev.data.fd = lfd;
	ev.events = EPOLLIN;
	epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);

	int nready;
	struct epoll_event events[1024];
	int cfd;
	int i;
	int sockfd;
	while (1)
	{
		// 等待事件发生
		nready = epoll_wait(epfd, events, 1024, -1);
		if (nready < 0)
		{
			if (nready == EINTR)
			{
				continue;
			}
			break;
		}

		for (i = 0; i < nready; i++)
		{
			sockfd = events[i].data.fd;
			// 客户端请求连接
			if (lfd == sockfd)
			{
				cfd = Accept(lfd, NULL, NULL);
				
				int flag = fcntl(cfd, F_GETFL);
				flag |= O_NONBLOCK;
				fcntl(cfd, F_SETFL, flag);

				// 将cfd上树
				ev.data.fd = cfd;
				ev.events = EPOLLIN;
				epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
			}
			else
			{
				// 客户端发送数据
				http_request(sockfd, epfd);
			}

		}
	}
}

int send_hander(int cfd, char* code, char* msg, char* fileType, int len)
{
	// 生成响应头
	char buf[1024] = {0};
	sprintf(buf, "HTTP/1.1 %s %s\r\n", code, msg);
	sprintf(buf + strlen(buf), "Content-Type:%s\r\n", fileType);
	if (len > 0)
	{
		sprintf(buf + strlen(buf), "Content-Length:%d\r\n", len);
	}
	strcat(buf, "\r\n");
	//printf("%s\n", buf);

	// 发送响应头
	Write(cfd, buf, strlen(buf));
	printf("send_headler\n");
	return 0;
}

int send_file(int cfd, char * fileName)
{
	// 读文件
	int fd = open(fileName, O_RDONLY);
	if (fd < 0)
	{
		perror("open error");
		return -1;
	}

	char buf[1024];
	int n;
	while (1)
	{
		memset(buf, 0x00, sizeof(buf));
		n = read(fd, buf, sizeof(buf));
		if (n <= 0)
		{
			break;
		}
		else
		{
			// 发送响应体
			write(cfd, buf, n);
		}
	}

	printf("send_file\n");
	return 0;
}

// http请求重定向（防止重复提交表单）
int send_redirect(int cfd, const char* url)
{
	char buf[1024];

	// 构建 HTTP 302 重定向响应
	sprintf(buf, "HTTP/1.1 302 Found\r\n");
	sprintf(buf + strlen(buf), "Location: %s\r\n", url); // 添加重定向 URL
	sprintf(buf + strlen(buf), "Content-Length: 0\r\n");
	sprintf(buf + strlen(buf), "Connection: close\r\n");
	strcat(buf, "\r\n"); // 空行结束头部

	// 发送响应
	Write(cfd, buf, strlen(buf));

	return 0;
}

// 解析客户端发来的请求并处理
int http_request(int cfd, int epfd)
{
	char buf[1024];
	int n;

	memset(buf, 0x00, sizeof(buf));
	// 读请求头首行
	n = Readline(cfd, buf, sizeof(buf));
	if (n <= 0)
	{
		printf("file read over:[%d]\n", n);
		close(cfd);
		epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
		return -1;
	}
	printf("\n");
	printf("buf:[%s]\n", buf);

	// 分别为请求类型，文件名，协议
	char reqType[16] = { 0 };
	char fileName[255] = { 0 };
	char protocal[16] = { 0 };
	sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", reqType, fileName, protocal);
	
	// 打印信息
	printf("reqType:[%s]\n", reqType);
	printf("fileName:[%s]\n", fileName);
	printf("protocal:[%s]\n", protocal);

	// 处理 /login 请求
	if (strcmp(fileName, "/login") == 0 && strcmp(reqType, "POST") == 0)
	{
		// 读取 POST 请求体中的用户名和密码
		char username[50], password[50];
		while ((n = Readline(cfd, buf, sizeof(buf))) > 0)
		{
			// 寻找空行，空行之后为 POST 数据
			if (strcmp(buf, "\r\n") == 0)
			{
				// 读请求体
				n = Readline(cfd, buf, sizeof(buf));
				// 解析POST数据
				printf("POST data: %s\n", buf);

				// 解析POST数据
				sscanf(buf, "username=%[^&]&password=%s", username, password);
				
				strncpy(password, password, 6);
				password[6] = '\0';

				/*printf("Username: %s\n", username);
				printf("Password: %s\n", password);*/

				// 简单的身份验证（可以改为查数据库或其他方式）
				if (strcmp(username, "hyh") == 0 && strcmp(password, "123456") == 0)
				{
					// 登录成功后重定向到首页
					send_redirect(cfd, "/");
					/*const char* success_html = "<html><body><h1>Login successful!</h1></body></html>";
					Write(cfd, success_html, strlen(success_html));*/
					strcpy(fileName, "/");
					break;
				}
				else
				{
					// 生成并发送失败的响应
					send_hander(cfd, "401", "Unauthorized", get_mime_type(".html"), 0);
					const char* fail_html = "<html><body><h1>Login failed. Invalid credentials.</h1></body></html>";
					Write(cfd, fail_html, strlen(fail_html));
				}

				close(cfd);
				epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
				return 0;
			}

		}
	}

	// 文件名预处理
	char* pFile = fileName;
	if (strlen(fileName) <= 1)  // 如果是当前目录
	{
		strcpy(pFile, "./");
	}
	else
	{
		pFile = fileName + 1;
	}
	// 编码中文
	strdecode(pFile, pFile);

	printf("process fileName:[%s]\n", pFile);

	// 将剩余数据全部读完，防止粘包
	while ((n = Readline(cfd, buf, sizeof(buf))) > 0);

	struct stat st;
	if (stat(pFile, &st) < 0) // 如果文件不存在
	{
		printf("file is not exist\n");

		// 生成并发送响应头给客户端
		send_hander(cfd, "404", "NOT FOUND", get_mime_type(".html"), 0);

		// 生成并发送响应体给客户端
		send_file(cfd, "error.html");
	}
	else  // 如果文件存在
	{
		//printf("file is exist\n");
		if (S_ISREG(st.st_mode)) // 普通文件
		{
			printf("file is reg\n");

			// 生成并发送响应头给客户端
			send_hander(cfd, "200", "OK", get_mime_type(pFile), st.st_size);

			// 生成并发送响应体给客户端
			send_file(cfd, pFile);
		}
		else if (S_ISDIR(st.st_mode)) // 目录文件
		{
			printf("file is dir\n");

			// 生成并发送响应头给客户端
			send_hander(cfd, "200", "OK", get_mime_type(".html"), 0);

			// 生成并发送响应体头部给客户端
			send_file(cfd, "html/dir_header.html");

			// 对目录的所有文件进行列出
			char buffer[1024];
			struct dirent** namelist;
			int num;

			num = scandir(pFile, &namelist, NULL, alphasort);
			if (num < 0)
			{
				perror("scandir error");
				close(cfd);
				epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
				return -1;
			}
			else
			{
				while (num--)
				{
					//printf("%s\n", namelist[num]->d_name);
					memset(buffer, 0x00, sizeof(buffer));
					if (namelist[num]->d_type == DT_DIR)
					{
						sprintf(buffer, "<li><a href=%s/>%s</a></li>", namelist[num]->d_name, namelist[num]->d_name);
					}
					else
					{
						sprintf(buffer, "<li><a href=%s>%s</a></li>", namelist[num]->d_name, namelist[num]->d_name);
					}
					free(namelist[num]);
					Write(cfd, buffer, strlen(buffer));
				}
				free(namelist);
			}

			// 生成并发送响应体尾部给客户端
			send_file(cfd, "html/dir_tail.html");
		}
	}
}

