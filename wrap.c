#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
//�󶨴�����ʾ���˳�
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
		if ((errno == ECONNABORTED) || (errno == EINTR))//ECONNABORTED ��������ʧ�� ETINTR �����źŴ��
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
		if (errno == EINTR)//���źŴ��Ӧ�ü�����
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

/*����: Ӧ�ö�ȡ���ֽ���*/
ssize_t Readn(int fd, void* vptr, size_t n)
{
	size_t  nleft;              //usigned int ʣ��δ��ȡ���ֽ���
	ssize_t nread;              //int ʵ�ʶ������ֽ���
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

		nleft -= nread;//��ֹһ������û�ж���
		ptr += nread;//ָ����Ҫ����ƶ�
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
	static char read_buf[100];//������100�Ļ�����

	if (read_cnt <= 0) {
	again:
		//ʹ�û��������Ա����δӵײ㻺���ȡ����--Ϊ�����Ч��
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
	*ptr = *read_ptr++;//�ӻ�����ȡ����

	return 1;
}
//��ȡһ��
ssize_t Readline(int fd, void* vptr, size_t maxlen)
{
	ssize_t n, rc;
	char    c, * ptr;

	ptr = (char *)vptr;
	for (n = 1; n < maxlen; n++) {
		if ((rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')//�����������
				break;
		}
		else if (rc == 0) {//�Զ˹ر�
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
	bzero(&serv_addr, sizeof(serv_addr));//���serv_addr��ַ �Ա� memset()
	if (IP == NULL) {
		//�������ʹ�� 0.0.0.0,����ip����������
		serv_addr.sin_addr.s_addr = INADDR_ANY;
	}
	else {
		if (inet_pton(AF_INET, IP, &serv_addr.sin_addr.s_addr) <= 0) {
			perror(IP);//ת��ʧ��
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

