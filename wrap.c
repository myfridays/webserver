#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
//绑定错误显示和退出
void perr_exit(const char* s)
{
	perror(s);
	exit(-1);
}

int Accept(int fd, struct sockaddr* sa, socklen_t* salenptr)
{
	int n;

again:
	if ((n = accept(fd, sa, salenptr)) < 0) {
		if ((errno == ECONNABORTED) || (errno == EINTR))//ECONNABORTED 代表连接失败 ETINTR 代表被信号打断
			goto again;
		else
			perr_exit("accept error");
	}
	return n;
}

int Bind(int fd, const struct sockaddr* sa, socklen_t salen)
{
	int n;

	if ((n = bind(fd, sa, salen)) < 0)
		perr_exit("bind error");

	return n;
}

int Connect(int fd, const struct sockaddr* sa, socklen_t salen)
{
	int n;

	if ((n = connect(fd, sa, salen)) < 0)
		perr_exit("connect error");

	return n;
}

int Listen(int fd, int backlog)
{
	int n;

	if ((n = listen(fd, backlog)) < 0)
		perr_exit("listen error");

	return n;
}

int Socket(int family, int type, int protocol)
{
	int n;

	if ((n = socket(family, type, protocol)) < 0)
		perr_exit("socket error");

	return n;
}

ssize_t Read(int fd, void* ptr, size_t nbytes)
{
	ssize_t n;

again:
	if ((n = read(fd, ptr, nbytes)) == -1) {
		if (errno == EINTR)//被信号打断应该继续读
			goto again;
		else
			return -1;
	}
	return n;
}

ssize_t Write(int fd, const void* ptr, size_t nbytes)
{
	ssize_t n;

again:
	if ((n = write(fd, ptr, nbytes)) == -1) {
		if (errno == EINTR)
			goto again;
		else
			return -1;
	}
	return n;
}

int Close(int fd)
{
	int n;
	if ((n = close(fd)) == -1)
		perr_exit("close error");

	return n;
}

/*参三: 应该读取的字节数*/
ssize_t Readn(int fd, void* vptr, size_t n)
{
	size_t  nleft;              //usigned int 剩余未读取的字节数
	ssize_t nread;              //int 实际读到的字节数
	char* ptr;

	ptr = vptr;
	nleft = n;

	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;
			else
				return -1;
		}
		else if (nread == 0)
			break;

		nleft -= nread;//防止一次数据没有读完
		ptr += nread;//指针需要向后移动
	}
	return n - nleft;
}

ssize_t Writen(int fd, const void* vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char* ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				return -1;
		}

		nleft -= nwritten;
		ptr += nwritten;
	}
	return n;
}

static ssize_t my_read(int fd, char* ptr)
{
	static int read_cnt;
	static char* read_ptr;
	static char read_buf[100];//定义了100的缓冲区

	if (read_cnt <= 0) {
	again:
		//使用缓冲区可以避免多次从底层缓冲读取数据--为了提高效率
		if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return -1;
		}
		else if (read_cnt == 0)
			return 0;
		read_ptr = read_buf;
	}
	read_cnt--;
	*ptr = *read_ptr++;//从缓冲区取数据

	return 1;
}
//读取一行
ssize_t Readline(int fd, void* vptr, size_t maxlen)
{
	ssize_t n, rc;
	char    c, * ptr;

	ptr = (char *)vptr;
	for (n = 1; n < maxlen; n++) {
		if ((rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')//代表任务完成
				break;
		}
		else if (rc == 0) {//对端关闭
			*ptr = 0;//0 = '\0'
			return n - 1;
		}
		else
			return -1;
	}
	*ptr = 0;

	return n;
}

int tcp4bind(short port, const char* IP)
{
	struct sockaddr_in serv_addr;
	int lfd = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&serv_addr, sizeof(serv_addr));//清空serv_addr地址 对比 memset()
	if (IP == NULL) {
		//如果这样使用 0.0.0.0,任意ip将可以连接
		serv_addr.sin_addr.s_addr = INADDR_ANY;
	}
	else {
		if (inet_pton(AF_INET, IP, &serv_addr.sin_addr.s_addr) <= 0) {
			perror(IP);//转换失败
			exit(1);
		}
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	int opt = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	Bind(lfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	return lfd;
}

