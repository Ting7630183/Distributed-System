#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <dirtree.h>

int sockfd;
//Build connection to the server
void connectServer(){
	char *serverip;
	char *serverport;
	unsigned short port;
	// char *msg="Hello from client";
	// char buf[MAXMSGLEN+1];
	// int sockfd, rv;
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
	mode_t m=0;
	if (flags & O_CREAT) {
		va_list a;
		va_start(a, flags);
		m = va_arg(a, mode_t);
		va_end(a);
	}
	// we just print a message, then call through to the original open function (from libc)
	//fprintf(stderr, "open\n");
	send(sockfd, "open\n", 5, 0);
	return orig_open(pathname, flags, m);
}
// This is our replacemnt for the read function from libc.
ssize_t read(int fd, void *buf, size_t count){
	//fprintf(stderr, "read\n");
	send(sockfd, "read\n", 5, 0);
	return orig_read(fd, buf, count);
}

//This is our replacemnt for the write function from libc.
ssize_t write(int fd, const void *buf, size_t count){
	//fprintf(stderr, "write\n");
	send(sockfd, "write\n", 6, 0);
	return orig_write(fd, buf, count);
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
int close(int open_fd){+
	//fprintf(stderr, "close\n");
	send(sockfd, "close\n", 6, 0);
	return orig_close(open_fd);
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
	connectServer();
	// set function pointer orig_open to point to the original open function
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


