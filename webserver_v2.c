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
	// ���������͵����ݵ�ʱ�򣬵��ͻ��˹ر�����
	// ����������SIGPIPE�ź�
	struct sigaction act;
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGPIPE, &act, NULL);

	// �ı䵱ǰ�Ĺ���Ŀ¼
	char path[1024];
	sprintf(path, "%s/%s", getenv("HOME"), "projects/webserver");
	chdir(path);
	printf("work__path:%s\n", path);
	// ����socket--�˿ڸ���--bind
	int lfd = tcp4bind(9999, NULL);

	// ����
	Listen(lfd, 128);

	// ����epoll��
	int epfd = epoll_create(1024);
	if (epfd < 0)
	{
		perror("epoll_create error");
		return -1;
	}

	// ��������������
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
		// �ȴ��¼�����
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
			// �ͻ�����������
			if (lfd == sockfd)
			{
				cfd = Accept(lfd, NULL, NULL);
				
				int flag = fcntl(cfd, F_GETFL);
				flag |= O_NONBLOCK;
				fcntl(cfd, F_SETFL, flag);

				// ��cfd����
				ev.data.fd = cfd;
				ev.events = EPOLLIN;
				epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
			}
			else
			{
				// �ͻ��˷�������
				http_request(sockfd, epfd);
			}

		}
	}
}

int send_hander(int cfd, char* code, char* msg, char* fileType, int len)
{
	// ������Ӧͷ
	char buf[1024] = {0};
	sprintf(buf, "HTTP/1.1 %s %s\r\n", code, msg);
	sprintf(buf + strlen(buf), "Content-Type:%s\r\n", fileType);
	if (len > 0)
	{
		sprintf(buf + strlen(buf), "Content-Length:%d\r\n", len);
	}
	strcat(buf, "\r\n");
	//printf("%s\n", buf);

	// ������Ӧͷ
	Write(cfd, buf, strlen(buf));
	printf("send_headler\n");
	return 0;
}

int send_file(int cfd, char * fileName)
{
	// ���ļ�
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
			// ������Ӧ��
			write(cfd, buf, n);
		}
	}

	printf("send_file\n");
	return 0;
}

// http�����ض��򣨷�ֹ�ظ��ύ����
int send_redirect(int cfd, const char* url)
{
	char buf[1024];

	// ���� HTTP 302 �ض�����Ӧ
	sprintf(buf, "HTTP/1.1 302 Found\r\n");
	sprintf(buf + strlen(buf), "Location: %s\r\n", url); // ����ض��� URL
	sprintf(buf + strlen(buf), "Content-Length: 0\r\n");
	sprintf(buf + strlen(buf), "Connection: close\r\n");
	strcat(buf, "\r\n"); // ���н���ͷ��

	// ������Ӧ
	Write(cfd, buf, strlen(buf));

	return 0;
}

// �����ͻ��˷��������󲢴���
int http_request(int cfd, int epfd)
{
	char buf[1024];
	int n;

	memset(buf, 0x00, sizeof(buf));
	// ������ͷ����
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

	// �ֱ�Ϊ�������ͣ��ļ�����Э��
	char reqType[16] = { 0 };
	char fileName[255] = { 0 };
	char protocal[16] = { 0 };
	sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", reqType, fileName, protocal);
	
	// ��ӡ��Ϣ
	printf("reqType:[%s]\n", reqType);
	printf("fileName:[%s]\n", fileName);
	printf("protocal:[%s]\n", protocal);

	// ���� /login ����
	if (strcmp(fileName, "/login") == 0 && strcmp(reqType, "POST") == 0)
	{
		// ��ȡ POST �������е��û���������
		char username[50], password[50];
		while ((n = Readline(cfd, buf, sizeof(buf))) > 0)
		{
			// Ѱ�ҿ��У�����֮��Ϊ POST ����
			if (strcmp(buf, "\r\n") == 0)
			{
				// ��������
				n = Readline(cfd, buf, sizeof(buf));
				// ����POST����
				printf("POST data: %s\n", buf);

				// ����POST����
				sscanf(buf, "username=%[^&]&password=%s", username, password);
				
				strncpy(password, password, 6);
				password[6] = '\0';

				/*printf("Username: %s\n", username);
				printf("Password: %s\n", password);*/

				// �򵥵������֤�����Ը�Ϊ�����ݿ��������ʽ��
				if (strcmp(username, "hyh") == 0 && strcmp(password, "123456") == 0)
				{
					// ��¼�ɹ����ض�����ҳ
					send_redirect(cfd, "/");
					/*const char* success_html = "<html><body><h1>Login successful!</h1></body></html>";
					Write(cfd, success_html, strlen(success_html));*/
					strcpy(fileName, "/");
					break;
				}
				else
				{
					// ���ɲ�����ʧ�ܵ���Ӧ
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

	// �ļ���Ԥ����
	char* pFile = fileName;
	if (strlen(fileName) <= 1)  // ����ǵ�ǰĿ¼
	{
		strcpy(pFile, "./");
	}
	else
	{
		pFile = fileName + 1;
	}
	// ��������
	strdecode(pFile, pFile);

	printf("process fileName:[%s]\n", pFile);

	// ��ʣ������ȫ�����꣬��ֹճ��
	while ((n = Readline(cfd, buf, sizeof(buf))) > 0);

	struct stat st;
	if (stat(pFile, &st) < 0) // ����ļ�������
	{
		printf("file is not exist\n");

		// ���ɲ�������Ӧͷ���ͻ���
		send_hander(cfd, "404", "NOT FOUND", get_mime_type(".html"), 0);

		// ���ɲ�������Ӧ����ͻ���
		send_file(cfd, "error.html");
	}
	else  // ����ļ�����
	{
		//printf("file is exist\n");
		if (S_ISREG(st.st_mode)) // ��ͨ�ļ�
		{
			printf("file is reg\n");

			// ���ɲ�������Ӧͷ���ͻ���
			send_hander(cfd, "200", "OK", get_mime_type(pFile), st.st_size);

			// ���ɲ�������Ӧ����ͻ���
			send_file(cfd, pFile);
		}
		else if (S_ISDIR(st.st_mode)) // Ŀ¼�ļ�
		{
			printf("file is dir\n");

			// ���ɲ�������Ӧͷ���ͻ���
			send_hander(cfd, "200", "OK", get_mime_type(".html"), 0);

			// ���ɲ�������Ӧ��ͷ�����ͻ���
			send_file(cfd, "html/dir_header.html");

			// ��Ŀ¼�������ļ������г�
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

			// ���ɲ�������Ӧ��β�����ͻ���
			send_file(cfd, "html/dir_tail.html");
		}
	}
}

