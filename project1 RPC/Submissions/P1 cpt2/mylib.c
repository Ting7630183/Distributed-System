#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <dirtree.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#define MAXMSGLEN 1024
#define FD_OFFSET 7000

int sockfd;
//int open_fd_from_server;

//Build connection to the server
void connectServer(){
	char *serverip;
	char *serverport;
	unsigned short port;
	int rv;
	struct sockaddr_in srv;
	
	// Get environment variable indicating the ip address of the server
	serverip = getenv("server15440");
	
	if (!serverip){
		serverip = "127.0.0.1";
	}

	// Get environment variable indicating the port of the server
	serverport = getenv("serverport15440");
	if(!serverport){
		serverport = "15440";
	}
	port = (unsigned short)atoi(serverport);
	
	// Create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);	// TCP/IP socket
	if (sockfd<0) err(1, 0);			// in case of error
	
	// setup address structure to point to server
	memset(&srv, 0, sizeof(srv));			// clear it first
	srv.sin_family = AF_INET;			// IP family
	srv.sin_addr.s_addr = inet_addr(serverip);	// IP address of server
	srv.sin_port = htons(port);			// server port

	// actually connect to the server
	rv = connect(sockfd, (struct sockaddr*)&srv, sizeof(struct sockaddr));
	if (rv<0) err(1,0);
	// send(sockfd, msg, strlen(msg), 0);	// send message; should check return value	
}

// The following line declares a function pointer with the same prototype as the open function.  
int (*orig_open)(const char *pathname, int flags, ...);  // mode_t mode is needed when flags includes O_CREAT
ssize_t (*orig_read)(int fd, void *buf, size_t count);
ssize_t (*orig_write)(int fd, const void *buf, size_t count);
off_t (*orig_lseek)(int fd, off_t offset, int whence);
int (*orig___xstat)(int ver, const char *filename, struct stat *statbuf);
int (*orig_unlink)(const char *pathname);
ssize_t (*orig_getdirentries)(int fd, char *buf, size_t nbytes, off_t *basep);
int (*orig_close)(int open_fd);
struct dirtreenode* (*orig_getdirtree)( const char *path);
void (*orig_freedirtree) ( struct dirtreenode* dt );


// This is our replacement for the open function from libc.
int open(const char *pathname, int flags, ...) {
	//printf("open\n");
	connectServer();
	mode_t m = 0;
	if (flags & O_CREAT) {
		va_list a;
		va_start(a, flags);
		m = va_arg(a, mode_t);
		va_end(a);
	}
	//printf("the mode is: %d\n", m);
    int total_size = sizeof(int) + sizeof(int) + sizeof(int) + strlen(pathname) * sizeof(char) + sizeof(int) + sizeof(mode_t) + 1;
	//printf("the total size send to server is: %d\n", total_size);
	char *buffer = malloc(total_size);

    int offset = 0;
	int size = sizeof(int);
	memcpy(buffer+offset, &total_size, size);
	int copy;
	memcpy(&copy, buffer+offset, size);
	//printf("the total size send to server is:%d\n", copy);
	
	int upcode = 1;
	offset += size;
	size = sizeof(int);
	memcpy(buffer+offset, &upcode, size);
	int copy1;
	memcpy(&copy1, buffer+offset, size);
	//printf("the upcode is:%d\n", copy1);
	
	offset += size;
	size = sizeof(mode_t);
	memcpy(buffer+offset, &m, size);
	
	offset += size;
	size = sizeof(int);
	memcpy(buffer+offset, &flags, size);

	int size_of_pathname = strlen(pathname) * sizeof(char);
	//printf("size of pathname in the client side is: %d\n", size_of_pathname);
	offset += size;
	//printf("the offset is: %d\n", offset);
	size = sizeof(int);
	//printf("the size is......:%d\n", size);
	memcpy(buffer+offset, &size_of_pathname, size);
	
	offset += size;
	size = size_of_pathname * sizeof(char);
	memcpy(buffer+offset, pathname, size);
	buffer[total_size-1] = 0;

	//printf("flags in the client side is:%d\n", flags);

	send(sockfd, buffer, total_size, 0);

	char buf[MAXMSGLEN+1];
	int rv = recv(sockfd, buf, MAXMSGLEN, 0);	// get message
	//printf("the number of byte receive by client in open method is:%d\n", rv);
	if (rv<0) err(1,0);			// in case something went wrong
	//buf[rv]=0;		// null terminate string to print

	int upcode_send_from_server;
	int offset2 = 0;
	int size2 = sizeof(int);
	memcpy(&upcode_send_from_server, buf+offset2, size2);
	//printf("the upcode on the open method in client is:.....%d\n", upcode_send_from_server);

	int server_fd;
	offset2 += size2;
	memcpy(&server_fd, buf+offset2, size2);
	//printf("open_fd sent from the server is: %d\n", server_fd);

	int err;
	offset2 += size2;
	memcpy(&err, buf+offset2, size2);
	//printf("open_error sent from server is: %d\n", err);
	errno = err;
	orig_close(sockfd);
	return server_fd;
}

// This is our replacemnt for the read function from libc.
ssize_t read(int fd, void *buf, size_t count){
	//fprintf(stderr, "read\n");
	send(sockfd, "read\n", 5, 0);
	return orig_read(fd, buf, count);
}

//This is our replacemnt for the write function from libc.
ssize_t write(int fd, const void *buf, size_t count){
	connectServer();
	//printf("enter write\n");
	if(fd - FD_OFFSET < 0){
		return orig_write(fd, buf, count);
	}
    int total_size = sizeof(int) + sizeof(int) + sizeof(int) + sizeof(size_t) + count + 1;
	char *buffer = malloc(total_size);

	int offset = 0;
	int size = sizeof(int);
	memcpy(buffer+offset, &total_size, size);
	int copy;
	memcpy(&copy, buffer+offset, size);
	//printf("the total size send to server in write is:%d\n", copy);

	int upcode = 2;
	offset += size;
	memcpy(buffer+offset, &upcode, size);
	//printf("the upcode of write is: %d\n", upcode);

	offset += size;
	memcpy(buffer+offset, &fd, size);
	//printf("the fd in the write is: %d\n", fd);

	offset += size;
	memcpy(buffer+offset, &count, size);
	//printf("count in the write is:%ld\n", count);

	offset += size;
	size = count;
	memcpy(buffer+offset, buf, count);
	buffer[total_size-1] = 0;
	//printf("the line before send in write method\n");
	send(sockfd, buffer, total_size, 0);
	//printf("the line after the send in write\n");

	char buf2[MAXMSGLEN+1];
	int rv = recv(sockfd, buf2, MAXMSGLEN, 0);	// get message
	//printf("the number of byte received from server is:%d\n", rv);
	if (rv<0) err(1,0);			// in case something went wrong
	//buf2[rv]=0;		// null terminate string to print

	int offset2 = 0;
	int server_write_return;
	int size2 = sizeof(ssize_t);
	memcpy(&server_write_return, buf2+offset2, size2);
	//printf("write_return is: %d\n", server_write_return);

	int err;
	offset2 += size2;;
	size2 = sizeof(int);
	memcpy(&err, buf2+offset2, size2);
	//printf("write_error is: %d\n", err);
	errno = err;
	return server_write_return;
	orig_close(sockfd);
}

//This is our replacemnt for the lseek function from libc.
off_t lseek(int fd, off_t offset, int whence){
	//fprintf(stderr, "lseek\n");
	send(sockfd, "lseek\n", 6, 0);
	return orig_lseek(fd, offset, whence);
}

//This is our replacemnt for the stat function from libc.
int __xstat(int ver, const char *filename, struct stat *statbuf){
	//fprintf(stderr, "stat\n");
	send(sockfd, "__xstat\n", 8, 0);
	return orig___xstat(ver, filename, statbuf);
}

//This is our replacemnt for the getdirentries function from libc.
ssize_t getdirentries(int fd, char *buf, size_t nbytes, off_t *basep){
	//fprintf(stderr, "getdirentries\n");
	send(sockfd, "getdirentries\n", 14, 0);
	return orig_getdirentries(fd, buf, nbytes, basep);
}

//This is our replacemnt for the close function from libc.
int close(int open_fd){
	connectServer();
	if(open_fd - FD_OFFSET < 0){
		return orig_close(open_fd);
		//printf("close enter the local orgin_close\n");
	}
	//printf("fd is not local, has to send to server\n");
	int total_size = sizeof(int) + sizeof(int) + sizeof(int) + 1;
	char *buffer = malloc(total_size);

    int offset = 0;
	int size = sizeof(int);
	memcpy(buffer+offset, &total_size, size);

    int upcode = 3;
	offset += size;
	memcpy(buffer+offset, &upcode, size);
	int copy;
	memcpy(&copy, buffer+offset, size);
	//printf("the upcode of write is: %d\n", copy);

	offset += size;
	memcpy(buffer+offset, &open_fd, size);
	int copy2;
	memcpy(&copy2, buffer+offset, size);
	//printf("the fd for close is: %d\n", copy2);
	buffer[total_size-1] = 0;
	send(sockfd, buffer, total_size, 0);

	char buf[MAXMSGLEN+1];
	int rv = recv(sockfd, buf, MAXMSGLEN, 0);	// get message
	//printf("the number of byte receive by client is:%d\n", rv);
	if (rv<0) err(1,0);			// in case something went wrong
	//buf[rv]=0;		// null terminate string to print

	int offset2 = 0;
	int closd_result_from_server;
	int size2 = sizeof(int);
	memcpy(&closd_result_from_server, buf+offset2, size2);

	int err;
	offset2 += size2;
	memcpy(&err, buf+offset2, size2);
	//printf("error in the close is: %d\n", err);
	errno = err;
    orig_close(sockfd);
	return closd_result_from_server;
}

//This is our replacemnt for the unlink function from libc.
int unlink(const char *pathname){
	//fprintf(stderr, "unlink\n");
	send(sockfd, "unlink\n", 7, 0);
	return orig_unlink(pathname);
}

//This is our replacemnt for the getdirtree function from libc.
struct dirtreenode* getdirtree( const char *path ){
	//fprintf(stderr, "getdirtree\n");
	send(sockfd, "getdirtree\n", 11, 0);
	return orig_getdirtree(path);
}

//This is our replacemnt for the freedirtree function from libc.
void freedirtree( struct dirtreenode* dt ){
	//fprintf(stderr, "freedirtree\n");
	send(sockfd, "freedirtree\n", 12, 0);
	return orig_freedirtree(dt);
}

// This function is automatically called when program is started
void _init(void) {
	// set function pointer orig_** to point to the original functions
	orig_open = dlsym(RTLD_NEXT, "open");
	orig_read = dlsym(RTLD_NEXT, "read");
	orig_write = dlsym(RTLD_NEXT, "write");
	orig_lseek = dlsym(RTLD_NEXT, "lseek");
	orig___xstat = dlsym(RTLD_NEXT, "__xstat");
	orig_getdirentries = dlsym(RTLD_NEXT, "getdirentries");
	orig_close = dlsym(RTLD_NEXT, "close");
	orig_unlink = dlsym(RTLD_NEXT, "unlink");
	orig_getdirtree = dlsym(RTLD_NEXT, "getdirtree");
	orig_freedirtree = dlsym(RTLD_NEXT, "freedirtree");	
}


