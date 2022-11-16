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
#define OPEN 1
#define WRITE 2
#define CLOSE 3
#define READ 4
#define LSEEK 5
#define __XSTAT 6
#define UNLINK 7
#define GETDIRENTRIES 8

typedef struct{
    int fd;
    int error;
}pack_open;

typedef struct{
	int result;
	int error;
}pack_write;

typedef struct{
	int result;
	int error;
}pack_close;

typedef struct{
	int result;
	int error;
	char *buf;
}pack_read;

typedef struct{
	off_t off;
	int error;
}pack_lseek;

typedef struct{
	int result;
	int error;
	struct stat *statbuf;
}pack_x__stat;

typedef struct{
	int result;
	int error;
}pack_unlink;

typedef struct{
	int result;
	int error;
	char *buf;
} pack_getdirentries;


int sockfd;


/**
 * @brief build up connection between the client and the server.
 */
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
}

/**
 * @brief The function to serialize all the parameter of open, send to the server and receive 
 *        the file decriptor and errno back from server.
 * 
 * @param pathname the pathname of the file that is going to be opened.
 * @param flags the flags that defines the access mode of the file.
 * @param m the mode_t of the open.
 * @return pack_open a struct that contains file descriptor and errno of open from server.
 */
pack_open pack_unpack_open(const char *pathname, int flags, mode_t m){
    int total_size = sizeof(int) + sizeof(int) + sizeof(int) + strlen(pathname) * sizeof(char) + sizeof(int) + sizeof(mode_t) + 1;
	char *buffer = malloc(total_size);

    int offset = 0;
	int size = sizeof(int);
	memcpy(buffer+offset, &total_size, size);
	
	int upcode = OPEN;
	offset += size;
	size = sizeof(int);
	memcpy(buffer+offset, &upcode, size);
	
	offset += size;
	size = sizeof(mode_t);
	memcpy(buffer+offset, &m, size);
	
	offset += size;
	size = sizeof(int);
	memcpy(buffer+offset, &flags, size);

	int size_of_pathname = strlen(pathname) * sizeof(char);
	offset += size;
	size = sizeof(int);
	memcpy(buffer+offset, &size_of_pathname, size);
	
	offset += size;
	size = size_of_pathname;
	memcpy(buffer+offset, pathname, size);
	char copy[size_of_pathname];
	memcpy(copy, buffer+offset, size_of_pathname);
	buffer[total_size-1] = 0;

	send(sockfd, buffer, total_size, 0);
	free(buffer);

	char buf[MAXMSGLEN+1];
	int rv = recv(sockfd, buf, MAXMSGLEN, 0);	// get message
	if (rv<0) err(1,0);		// in case something went wrong

    int server_fd;
    int offset2 = 0;
	int size2 = sizeof(int);
	memcpy(&server_fd, buf+offset2, size2);

	int err;
	offset2 += size2;
	memcpy(&err, buf+offset2, size2);

	pack_open open_back;
	open_back.fd = server_fd;
	open_back.error = err;
	return open_back;	
}

/**
 * @brief The function to serialize all the parameter of write, send to the server and receive 
 *        the file decriptor and errno back from server.

 * @param fd the file descriptor of the file that is going to be written to.
 * @param buf  the buffer that the content will be written to.
 * @param count the number of bytes that is going to be written.
 * @return pack_write is a struct that contains the number of bytes that has been written and the errno from
 *         server. 
 */
pack_write pack_unpack_write(int fd, const void *buf, size_t count){
	int total_size = sizeof(int) + sizeof(int) + sizeof(int) + sizeof(size_t) + count + 1;
	char *buffer = malloc(total_size);

	int offset = 0;
	int size = sizeof(int);
	memcpy(buffer+offset, &total_size, size);
	
	int upcode = WRITE;
	offset += size;
	memcpy(buffer+offset, &upcode, size);

	offset += size;
	memcpy(buffer+offset, &fd, size);

	offset += size;
	size = sizeof(size_t);
	memcpy(buffer+offset, &count, size);

	offset += size;
	size = count;
	memcpy(buffer+offset, buf, count);
	buffer[total_size-1] = 0;

	send(sockfd, buffer, total_size, 0);
	free(buffer);

	char buf2[MAXMSGLEN+1];
	int rv = recv(sockfd, buf2, MAXMSGLEN, 0);	// get message
	if (rv<0) err(1,0);			// in case something went wrong

	int offset2 = 0;
	int server_write_return;
	int size2 = sizeof(ssize_t);
	memcpy(&server_write_return, buf2+offset2, size2);

	int err;
	offset2 += size2;;
	size2 = sizeof(int);
	memcpy(&err, buf2+offset2, size2);

	pack_write write_back;
	write_back.result = server_write_return;
	write_back.error = err;
	return write_back;
}

/**
 * @brief The function to serialize all the parameter of close, send to the server and receive 
 *        the close result and errno back from server.
 * 
 * @param fd the file descriptor of the file that is going to be closed.
 * @return pack_close is a struct that contains the Integer value of close result and the errno from server. 
 */
pack_close pack_unpack_close(int fd){
	int total_size = sizeof(int) + sizeof(int) + sizeof(int) + 1;
	char *buffer = malloc(total_size);

    int offset = 0;
	int size = sizeof(int);
	memcpy(buffer+offset, &total_size, size);

    int upcode = CLOSE;
	offset += size;
	memcpy(buffer+offset, &upcode, size);
	
	offset += size;
	memcpy(buffer+offset, &fd, size);

	buffer[total_size-1] = 0;
	send(sockfd, buffer, total_size, 0);
	free(buffer);

	char buf[MAXMSGLEN+1];
	int rv = recv(sockfd, buf, MAXMSGLEN, 0);	// get message
	if (rv<0) err(1,0);			// in case something went wrong
	//buf[rv]=0;		// null terminate string to print

	int offset2 = 0;
	int closd_result_from_server;
	int size2 = sizeof(int);
	memcpy(&closd_result_from_server, buf+offset2, size2);

	int err;
	offset2 += size2;
	memcpy(&err, buf+offset2, size2);

	pack_close close_back;
	close_back.error = err;
	close_back.result = closd_result_from_server;
	return close_back;
}

/**
 * @brief The function to serialize all the parameter of read, send to the server and receive 
 *        the close result and errno back from server.
 * 
 * @param fd the file descriptor of the file that is going to be read.
 * @param buf  the buffer that the content will be read.
 * @param count the number of bytes that is going to be read.
 * @return pack_read is a struct that contains the number of bytes that has been read and the errno from
 *         server. 
 */
pack_read pack_unpack_read(int fd, size_t count){
	int total_size = sizeof(int) + sizeof(int) + sizeof(int) + sizeof(size_t) + 1;
	char *buffer = malloc(total_size);

	int offset = 0;
	int size = sizeof(int);
	memcpy(buffer+offset, &total_size, size);
	int copy5;
	memcpy(&copy5, buffer+offset, size);
	
	int upcode = READ;
	offset += size;
	memcpy(buffer+offset, &upcode, size);

	offset += size;
	memcpy(buffer+offset, &fd, size);
	int copy;
	memcpy(&copy, buffer+offset, size);

	offset += size;
	size = sizeof(size_t);
	memcpy(buffer+offset, &count, size);
	size_t copy2;
	memcpy(&copy2, buffer+offset, size);

	buffer[total_size-1] = 0;
	send(sockfd, buffer, total_size, 0);
	free(buffer);

	char buf2[MAXMSGLEN+1];
	int rv = recv(sockfd, buf2, MAXMSGLEN, 0);	// get message
	if (rv<0) err(1,0);			// in case something went wrong
	//buf2[rv]=0;		// null terminate string to print

	int offset2 = 0;
	int server_read_return;
	int size2 = sizeof(ssize_t);
	memcpy(&server_read_return, buf2+offset2, size2);

	int err;
	offset2 += size2;;
	size2 = sizeof(int);
	memcpy(&err, buf2+offset2, size2);

	char *buf3 = malloc(server_read_return+1);
	offset2 += size2;
	size2 = server_read_return;
	memcpy(buf3, buf2+offset2, size2+1);
	
	pack_read read_back;
	read_back.result = server_read_return;
	read_back.error = err;
	read_back.buf = buf3;
	free(buf3); 
	return read_back;	
}
/**
 * @brief he function to serialize all the parameter of lseek, send to the server and receive 
 *        the close result and errno back from server.
 * 
 * @param fd the file descriptor of the file that is going to be written to.
 * @param offset the number that specifies the number of bytes to offset the 
 *               file pointer from a specified file origin.
 * @param whence the number that Specifies the location from which to start seeking. 
 * @return pack_lseek is a struct that contains the offset of the lseek and the errno from server. 
 */
pack_lseek pack_unpack_lseek(int fd, off_t offset, int whence){
	int total_size = sizeof(int) + sizeof(int) + sizeof(int) + sizeof(off_t) + sizeof(int);
	char *buffer = malloc(total_size);

	int offset_lseek = 0;
	int size = sizeof(int);
	memcpy(buffer+offset_lseek, &total_size, size);
	int copy;
	memcpy(&copy, buffer+offset_lseek, size);
	
	int upcode = LSEEK;
	offset_lseek += size;
	memcpy(buffer+offset_lseek, &upcode, size);

	offset_lseek += size;
	memcpy(buffer+offset_lseek, &fd, size);

	offset_lseek += size;
	size = sizeof(off_t);
	memcpy(buffer+offset_lseek, &offset, size);

	offset_lseek += size;
	size = sizeof(int);
	memcpy(buffer+offset_lseek, &whence, size);

	send(sockfd, buffer, total_size, 0);
	free(buffer);

	char buf[MAXMSGLEN+1];
	int rv = recv(sockfd, buf, MAXMSGLEN, 0);	// get message
	if (rv<0) err(1,0);			// in case something went wrong

	int offset2 = 0;
	int lseek_result_from_server;
	int size2 = sizeof(int);
	memcpy(&lseek_result_from_server, buf+offset2, size2);

	int err;
	offset2 += size2;
	memcpy(&err, buf+offset2, size2);

	pack_lseek lseek_back;
	lseek_back.error = err;
	lseek_back.off = lseek_result_from_server;
	return lseek_back;
}

/**
 * @brief he function to serialize all the parameter of __xstat, send to the server and receive 
 *        the close result and errno back from server.
 * 
 *  @param ver version number which shall be 3 or the behavior of these functions is undefined.
 * @param filename the file name.
 * @return pack_x__stat is a struct that contains the the result of calling __xstat on the server,
 *         the errno as well as the struct stat returned from the server.
 */
pack_x__stat pack_unpack_x__state(int ver, const char *filename){
	int total_size = sizeof(int) + sizeof(int) + sizeof(int) + sizeof(int) + strlen(filename) * sizeof(char) + 1;
	char *buffer = malloc(total_size);

	int offset = 0;
	int size = sizeof(int);
	memcpy(buffer+offset, &total_size, size);


	int upcode = __XSTAT;
	offset += size;
	memcpy(buffer+offset, &upcode, size);

	offset += size;
	memcpy(buffer+offset, &ver, size);

	int size_of_filename = strlen(filename) * sizeof(char);
	offset += size;
	size = sizeof(int);
	memcpy(buffer+offset, &size_of_filename, size);
	
	offset += size;
	size = size_of_filename;
	memcpy(buffer+offset, filename, size);
	buffer[total_size-1] = 0;
	send(sockfd, buffer, total_size, 0);
	free(buffer);

	char buf[MAXMSGLEN+1];
	int rv = recv(sockfd, buf, MAXMSGLEN, 0);	// get message
	if (rv<0) err(1,0);			// in case something went wrong

	int offset2 = 0;
	int _xstat_result_from_server;
	int size2 = sizeof(int);
	memcpy(&_xstat_result_from_server, buf+offset2, size2);

	offset2 += size2;
	int err;
	memcpy(&err, buf+offset2, size2);

	offset2 += size2;
	struct stat *statbuf = (struct stat*)malloc(sizeof(struct stat));
	memcpy(statbuf, buf+offset2, sizeof(struct stat));

	pack_x__stat stat_back;
	stat_back.result = _xstat_result_from_server;
	stat_back.error = err;
	stat_back.statbuf = statbuf;

	return stat_back;
}
/**
 * @brief he function to serialize all the parameter of unlink, send to the server and receive.
 *        the unlink result and errno back from server.
 * 
 * @param pathname the name of the file that will be deleted in the file system.
 * @return pack_unlink is a struct that contains the the result of calling unlink on the server and the errno.
 */
pack_unlink pack_unpack_unlink(const char *pathname){
	int total_size = sizeof(int) + sizeof(int) + sizeof(int) + strlen(pathname) * sizeof(char) + 1;
	char *buffer = malloc(total_size);

	int offset = 0;
	int size = sizeof(int);
	memcpy(buffer+offset, &total_size, size);
	int copy1;
	memcpy(&copy1, buffer+offset, size);

	int upcode = UNLINK;
	offset += size;
	memcpy(buffer+offset, &upcode, size);
	int copy2;
	memcpy(&copy2, buffer+offset, size);

	int size_of_pathname = strlen(pathname) * sizeof(char);
	offset += size;
	size = sizeof(int);
	memcpy(buffer+offset, &size_of_pathname, size);

	offset += size;
	size = size_of_pathname * sizeof(char);
	memcpy(buffer+offset, pathname, size);
	buffer[total_size-1] = 0;

	send(sockfd, buffer, total_size, 0);
	free(buffer);

	char buf[MAXMSGLEN+1];
	int rv = recv(sockfd, buf, MAXMSGLEN, 0);	// get message
	if (rv<0) err(1,0);			// in case something went wrong
	//buf[rv]=0;		// null terminate string to print

	int offset2 = 0;
	int unlink_result_from_server;
	int size2 = sizeof(int);
	memcpy(&unlink_result_from_server, buf+offset2, size2);

	offset2 += size2;
	int err;
	memcpy(&err,buf+offset2, size2);

	pack_unlink unlink_back;
	unlink_back.error = err;
	unlink_back.result = unlink_result_from_server;
	return unlink_back;
}

/**
 * @brief he function to serialize all the parameter of getdirentries, send to the server and receive.
 *        the getdirentries result and errno back from server.
 * @param fd the file descriptor.
 * @param nbytes the biggest possible number of bytes to read.
 * @param basep the reading start position. 
 * @return pack_getdirentries is a struct that contains the the result of calling getdirentries on the server, the errno
 *         and the directory information of the file.
 */
pack_getdirentries pack_unpack_direntries(int fd, size_t nbytes, off_t *basep){
	int total_size = sizeof(int) + sizeof(int) + sizeof(int) + sizeof(size_t) + sizeof(off_t) + 1;
	char *buffer = malloc(total_size);

	int offset = 0;
	int size = sizeof(int);
	memcpy(buffer+offset, &total_size, size);

	int upcode = GETDIRENTRIES;
	offset += size;
	memcpy(buffer+offset, &upcode, size);

	offset += size;
	memcpy(buffer+offset, &fd, size);

	offset += size;
	size = sizeof(size_t);
	memcpy(buffer+offset, &nbytes, size);

	offset += size;
	size = sizeof(off_t);
	memcpy(buffer+offset, basep, size);
	buffer[total_size-1] = 0;

	send(sockfd, buffer, total_size, 0);
	free(buffer);
    
	char buf[MAXMSGLEN+1];
	int rv = recv(sockfd, buf, MAXMSGLEN, 0);	// get message
	if (rv<0) err(1,0);			// in case something went wrong

	int offset2 = 0;
	int getdirentries_result_from_server;
	int size2 = sizeof(int);
	memcpy(&getdirentries_result_from_server, buf+offset2, size2);

	offset2 += size2;
	int err;
	memcpy(&err,buf+offset2, size2);

    char buf2[sizeof(off_t)];
	offset2 += size2;
	size2 = sizeof(off_t);
	memcpy(buf2, buf+offset2, size2);

	pack_getdirentries direntries_back;
	direntries_back.error = err;
	direntries_back.result = getdirentries_result_from_server;
	direntries_back.buf = buf2;
	return direntries_back;	
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

/**
 * @brief This is the replacement for the open function of libc.
 * 
 * @param pathname the pathname of the file that is going to be opened
 * @param flags the flags of the open
 * @param ...  the mode_t of the open
 * @return int the file descriptor of the file that has been opened
 */
int open(const char *pathname, int flags, ...) {
	mode_t m = 0;
	if (flags & O_CREAT) {
		va_list a;
		va_start(a, flags);
		m = va_arg(a, mode_t);
		va_end(a);
	}
	pack_open open_back = pack_unpack_open(pathname, flags, m);
    errno = open_back.error;
	return open_back.fd;
}

/**
 * @brief This is the replacement for the read function of libc.
 * 
 * @param fd the file descriptor of the file that is going to be read.
 * @param buf the buffer that is going to store the data read from file.
 * @param count the number of bytes that is going to be read.
 * @return ssize_t the number of bytes that has been read to the buffer.
 */
ssize_t read(int fd, void *buf, size_t count){
	if(fd - FD_OFFSET < 0){
		return orig_read(fd, buf, count);
	}
	pack_read read_back = pack_unpack_read(fd, count);
	errno = read_back.error;
	
	buf = malloc(read_back.result);
	memcpy(buf, read_back.buf, read_back.result);
	return read_back.result;
}

/**
 * @brief This is the replacement for the write function of libc.
 * 
 * @param fd the file descriptor of the file that is going to be written to.
 * @param buf  the buffer that the content will be written to.
 * @param count the number of bytes that is going to be written.
 * @return ssize_t the number of bytes that has been successfully be written to.
 */
ssize_t write(int fd, const void *buf, size_t count){
	if(fd - FD_OFFSET < 0){
		return orig_write(fd, buf, count);
	}
	pack_write write_back = pack_unpack_write(fd, buf, count);
	errno = write_back.error;
	return write_back.result;
}

/**
 * @brief This is the replacement for the lseek function of libc.
 * 
 * @param fd the file descriptor of the file that is going to be written to.
 * @param offset the number that specifies the number of bytes to offset the 
 *               file pointer from a specified file origin.
 * @param whence the number that Specifies the location from which to start seeking.
 * @return off_t  If successful, return a nonnegative integer that indicates the file 
 *                pointer value. On failure, return a value of -1.
 */
off_t lseek(int fd, off_t offset, int whence){
	if(fd - FD_OFFSET < 0){
		return orig_lseek(fd, offset, whence);
	}
	pack_lseek lseek_back = pack_unpack_lseek(fd, offset, whence);
	errno = lseek_back.error;
	return lseek_back.off;
}

/**
 * @brief This is the replacement for the __xstat function of libc.
 * 
 * @param ver version number which shall be 3 or the behavior of these functions is undefined.
 * @param filename the file name.
 * @param statbuf a struct stat to store the status of the file.
 * @return int return 0 on success, -1 on fail.
 */
int __xstat(int ver, const char *filename, struct stat *statbuf){
	pack_x__stat x__stat_back = pack_unpack_x__state(ver, filename);
	errno = x__stat_back.error;
	struct stat *statbuffer = x__stat_back.statbuf;
	memcpy(statbuf, statbuffer, sizeof(struct stat));
	return x__stat_back.result;
}

/**
 * @brief This is the replacement for the gendirentries function of libc.
 * 
 * @param fd the file descriptor.
 * @param buf the buffer to store the information.
 * @param nbytes the biggest possible number of bytes to read.
 * @param basep the reading start position.
 * @return ssize_t the number of bytes read or zero when at the end of the directory.  
 *          If an error occurs, -1 is returned, and errno is set to indicate the error.
 */
ssize_t getdirentries(int fd, char *buf, size_t nbytes, off_t *basep){
	if(fd - FD_OFFSET < 0){
		return orig_getdirentries(fd, buf, nbytes, basep);
	}
	pack_getdirentries direntries_back = pack_unpack_direntries(fd, nbytes, basep);
	errno = direntries_back.error;
	buf = malloc(direntries_back.result);
	memcpy(buf, direntries_back.buf, nbytes);
	return direntries_back.result;
}

/**
 * @brief This is the replacement for the close function of libc.
 * 
 * @param open_fd the file descriptor of the file that is going to be closed.
 * @return int the result of the close. 0 will be returned on success and -1 on fail.
 */
int close(int open_fd){
	if(open_fd - FD_OFFSET < 0){
		return orig_close(open_fd);
	}
	pack_close close_back = pack_unpack_close(open_fd);
	errno = close_back.error;
	return close_back.result;
}

/**
 * @brief This is the replacement for the unlink function of libc.
 * 
 * @param pathname the name of the file that will be deleted in the file system.
 * @return int On success, zero is returned. On error, -1 is returned, and
            errno is set to indicate the error.
 */
int unlink(const char *pathname){
	pack_unlink unlink_back = pack_unpack_unlink(pathname);
	errno = unlink_back.error;
	return unlink_back.result;
}

/**
 * @brief given the pathname, this method would recursivly descends a directory
 *        hierarchy and construct a data structure that represents the directory
 *        tree.
 * 
 * @param path the path of a file.
 * @return struct dirtreenode*  a tree data structure.
 */
struct dirtreenode* getdirtree( const char *path ){
	send(sockfd, "getdirtree\n", 11, 0);
	return orig_getdirtree(path);
}

/**
 * @brief free the directory of a tree
 * @param dt a dirtreenode structure
 */
void freedirtree( struct dirtreenode* dt ){
	send(sockfd, "freedirtree\n", 12, 0);
	return orig_freedirtree(dt);
}

/**
 * @brief This function is automatically called when program is started
 */
void _init(void) {
	connectServer();
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




