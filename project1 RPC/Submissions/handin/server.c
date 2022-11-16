#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#define FD_OFFSET 7000
#define MAXMSGLEN 1024
#define OPEN 1
#define WRITE 2
#define CLOSE 3
#define READ 4
#define LSEEK 5
#define __XSTAT 6
#define UNLINK 7
#define GETDIRENTRIES 8


/**
 * @brief Receive all the serilized data from TCP and put the data into a buffer
 * 
 * @param sessfd session fd
 * @param buf the array buffer to store all the data from TCP
 */
void receive_all(int sessfd, char *buf){
	int rv;
	int size = sizeof(int);
	int total_size;
	int total_receive = 0;

	//Get the size of the serialized string first.
	while((rv =recv(sessfd, buf, size-total_receive, 0)) > 0){
		total_receive += rv;
		if(total_receive >= size){
			break;
		}
	}
	if(rv == 0){
		exit(0);
	}
	int offset = 0;
	memcpy(&total_size, buf+offset, size);

    // Receive the rest of the serilized string and put them to the buffer
	while((rv=recv(sessfd, buf+total_receive, total_size - total_receive, 0)) > 0){
		total_receive += rv;
		if(total_receive >= total_size){
			break;
		}	
	}
}

/**
 * @brief Deserilize the data of open method in the buffer, do the operation and send the 
 *        information back to client.
 * 
 * @param sessfd session fd
 * @param buf the buffer which store the data from client.
 * @param offset the position where extraction data starts.
 */
void deserilize_and_process_open(int sessfd, char *buf, int offset){
	mode_t m;
	int size = sizeof(mode_t);
	memcpy(&m, buf+offset, size);

	int flags;
	offset += size;
	size = sizeof(int);
	memcpy(&flags, buf+offset, size);

	int size_of_pathname;
	offset += size;
	size = sizeof(int);
	memcpy(&size_of_pathname, buf+offset, size);

	char pathname[size_of_pathname+1];
	offset += size;
	size = size_of_pathname+1;
	memcpy(&pathname, buf+offset, size);

	int open_fd = open(pathname, flags, m) + FD_OFFSET;

	int total_size_out =  sizeof(int) + sizeof(int);
	char *buffer = malloc(total_size_out);
        
    int offset_out = 0;
	int size_out = sizeof(int);
	memcpy(buffer+offset_out, &open_fd, size_out);

	offset_out += size_out;
	int err = errno;
	memcpy(buffer+offset_out, &err, size_out);
	send(sessfd, buffer, total_size_out+1, 0); 
	free(buffer);
}

/**
 * @brief Deserilize the data of write method in the buffer, do the operation and send the 
 *        information back to client.
 * 
 * @param sessfd session fd
 * @param buf the buffer which store the data from client.
 * @param offset the position where extraction data starts.
 */
void deserilize_and_process_write(int sessfd, char *buf, int offset){
	int write_fd;
	int size = sizeof(int);
	memcpy(&write_fd, buf+offset, size);
	write_fd -= FD_OFFSET;

	int count;
	offset += size;
	size = sizeof(size_t);
	memcpy(&count, buf+offset, size);

	char *buffer_write = malloc(count);
	offset += size;
	memcpy(buffer_write, buf+offset, count);

	ssize_t write_return = write(write_fd, buffer_write, count);

	int total_size_out = sizeof(ssize_t) + sizeof(int);
	char *buffer = malloc(total_size_out);
		    
	int offset_out = 0;
	int size_out = sizeof(ssize_t);
	memcpy(buffer+offset_out, &write_return, size_out);

	offset_out += size_out;
	size_out = sizeof(int);
	int err = errno;
	memcpy(buffer+offset_out, &err, size_out);
	send(sessfd, buffer, total_size_out+1, 0); 
	free(buffer_write); // not sure about whether free in this position
	free(buffer);
}

/**
 * @brief Deserilize the data of close method in the buffer, do the operation and send the 
 *        information back to client.
 * 
 * @param sessfd session fd
 * @param buf the buffer which store the data from client.
 * @param offset the position where extraction data starts.
 */
void deserilize_and_process_close(int sessfd, char *buf, int offset){
	int close_fd;
	int size = sizeof(int);
	memcpy(&close_fd, buf+offset, size);
	close_fd -= FD_OFFSET;
	int close_result = close(close_fd);

	int total_size_out = sizeof(int) +  sizeof(int);
	char *buffer = malloc(total_size_out);
        
	int offset_out = 0;
	int size_out = sizeof(int);
	memcpy(buffer+offset_out, &close_result, size_out);

	offset_out += size_out;
	int err = errno;
	memcpy(buffer+offset_out, &err, size_out);
	send(sessfd, buffer, total_size_out+1, 0);
	free(buffer);
}

/**
 * @brief Deserilize the data of read method in the buffer, do the operation and send the 
 *        information back to client.
 * 
 * @param sessfd session fd
 * @param buf the buffer which store the data from client.
 * @param offset the position where extraction data starts.
 */
void deserilize_and_process_read(int sessfd, char *buf, int offset){
	int read_fd;
	int size = sizeof(int);
	memcpy(&read_fd, buf+offset, size);
	read_fd -= FD_OFFSET;

	size_t count;
	offset += size;
	size = sizeof(size_t);
	memcpy(&count, buf+offset, size);

	char *buffer_read = malloc(count+1);
	ssize_t read_return = read(read_fd, buffer_read, count);
	buffer_read[count] = 0;

	int total_size_out = sizeof(ssize_t) + sizeof(int) + count + 1;
	char *buffer = malloc(total_size_out);
		    
	int offset_out = 0;
	int size_out = sizeof(ssize_t);
	memcpy(buffer+offset_out, &read_return, size_out);

	offset_out += size_out;
	size_out = sizeof(int);
	int err = errno;
	memcpy(buffer+offset_out, &err, size_out);

	offset_out += size_out;
	size_out = count;
	memcpy(buffer+offset_out, buffer_read, count);

	send(sessfd, buffer, total_size_out, 0); 
	free(buffer_read);
	free(buffer);
}

/**
 *  @brief Deserilize the data of lseek method in the buffer, do the operation and send the 
 *         information back to client.
 * 
 * @param sessfd session fd
 * @param buf the buffer which store the data from client.
 * @param offset the position where extraction data starts.
 */
void deserilize_and_process_lseek(int sessfd, char *buf, int offset){
	int lseek_fd;
	int size = sizeof(int);
	memcpy(&lseek_fd, buf+offset, size);
	lseek_fd -= FD_OFFSET;

	off_t off;
	offset += size;
	size = sizeof(off_t);
	memcpy(&off, buf+offset, size);

	int whence;
	offset += size;
	size = sizeof(int);
	memcpy(&whence, buf+offset, size);

	off_t result = lseek(lseek_fd, off, whence);
	int total_size_out = sizeof(off_t) + sizeof(int);
	char *buffer = malloc(total_size_out);
		    
	int offset_out = 0;
	int size_out = sizeof(off_t);
	memcpy(buffer+offset_out, &result, size_out);

	offset_out += size_out;
	size_out = sizeof(int);
	int err = errno;
	memcpy(buffer+offset_out, &err, size_out);
	send(sessfd, buffer, total_size_out+1, 0); 
	free(buffer);
}

/**
 * @brief Deserilize the data of __xstat method in the buffer, do the operation and send the 
 *        information back to client.
 * 
 * @param sessfd session fd
 * @param buf the buffer which store the data from client.
 * @param offset the position where extraction data starts.
 */
void deserilize_and_process_x__stat(int sessfd, char *buf, int offset){
	int ver;
	int size = sizeof(int);
	memcpy(&ver, buf+offset, size);

	int size_filename;
	offset += size;
	memcpy(&size_filename, buf+offset, size);

	char filename[size_filename+1];
	offset += size;
	memcpy(filename, buf+offset, size_filename+1);

	struct stat *statbuf = (struct stat*)malloc(sizeof(struct stat));
	int result = __xstat(ver, filename, statbuf);
	int err = errno;

	int total_size_out = sizeof(int) + sizeof(int) + sizeof(struct stat);
	char *buffer = malloc(total_size_out);
		    
	int offset_out = 0;
	int size_out = sizeof(int);
	memcpy(buffer+offset_out, &result, size_out);

	offset_out += size_out;
	size_out = sizeof(int);
	memcpy(buffer+offset_out, &err, size_out);

	offset += size_out;
	size_out = sizeof(struct stat);
	memcpy(buffer+offset_out, statbuf, size_out);
	send(sessfd, buffer, total_size_out, 0); 
	free(statbuf);
	free(buffer);
}

/**
 * @brief Deserilize the data of unlink method in the buffer, do the operation and send the 
 *        information back to client.
 * 
 * @param sessfd session fd
 * @param buf the buffer which store the data from client.
 * @param offset the position where extraction data starts.
 */
void deserilize_and_process_unlink(int sessfd, char *buf, int offset){
	int size_pathname;
	int size = sizeof(int);
	memcpy(&size_pathname, buf+offset, size);

	char pathname[size_pathname];
	offset += size;
	size = size_pathname;
	memcpy(pathname, buf+offset, size);

	int result = unlink(pathname);

	int total_size_out = sizeof(int) + sizeof(int);
	char *buffer = malloc(total_size_out);
		    
	int offset_out = 0;
	int size_out = sizeof(int);
	memcpy(buffer+offset_out, &result, size_out);

	offset_out += size_out;
	size_out = sizeof(int);
	int err = errno;
	memcpy(buffer+offset_out, &err, size_out);
	send(sessfd, buffer, total_size_out+1, 0); 	
	free(buffer);
}

/**
 * @brief Deserilize the data of getdirentries method in the buffer, do the operation and send the 
 *        information back to client.
 * 
 * @param sessfd session fd
 * @param buf the buffer which store the data from client.
 * @param offset the position where extraction data starts.
 */
void deserilize_and_process_direntries(int sessfd, char *buf, int offset){
	int fd;
	int size = sizeof(int);
	memcpy(&fd, buf+offset, size);
	int copy1;
	memcpy(&copy1, buf+offset, size);

	size_t nbytes;
	offset += size;
	size = sizeof(size_t);
	memcpy(&nbytes, buf+offset, size);
	int copy2;
	memcpy(&copy2, buf+offset, size);

	off_t *basep = malloc(sizeof(basep));
	offset += size;
	size = sizeof(off_t);
	memcpy(basep, buf+offset, size);
	
	char buf_bytes[nbytes];
	ssize_t result = getdirentries(fd, buf_bytes, nbytes, basep);

	int total_size_out = sizeof(ssize_t) + sizeof(int);
	char *buffer = malloc(total_size_out);
		    
	int offset_out = 0;
	int size_out = sizeof(ssize_t);
	memcpy(buffer+offset_out, &result, size_out);

	offset_out += size_out;
	size_out = sizeof(int);
	int err = errno;
	memcpy(buffer+offset_out, &err, size_out);

	offset_out += size_out;
	size_out = nbytes;
	memcpy(buffer+offset_out, buf_bytes, size_out);
	send(sessfd, buffer, total_size_out+1, 0); 	
	free(buffer);
}


/**
 * @brief Receive all information from TCP, deserilized the data, do the corresponding operation 
 * and then send information back to client.
 * 
 * @param sessfd session fd.
 */
void receive_and_process(int sessfd){
	char buf[MAXMSGLEN+1];
	receive_all(sessfd, buf);

    //extract the upcode from the buffer
    int size = sizeof(int);
	int upcode;
	int offset = sizeof(int);
	size = sizeof(int);
	memcpy(&upcode, buf+offset, size);
		
	switch(upcode) {
		default:{
			break;
		}
	    case OPEN:{
			deserilize_and_process_open(sessfd, buf, offset+size);
		    break;
		}		   
	    case WRITE: {
			deserilize_and_process_write(sessfd, buf, offset+size);
		    break;
	    }
	    case CLOSE:{
			deserilize_and_process_close(sessfd, buf, offset+size);
		    break;
	    }
		case READ:{
			deserilize_and_process_read(sessfd, buf, offset+size);
			break;
		}
		case LSEEK:{
			deserilize_and_process_lseek(sessfd, buf, offset+size);
			break;
		}
		case __XSTAT:{
			deserilize_and_process_x__stat(sessfd, buf, offset+size);
			break;
		}	
		case UNLINK:{
			deserilize_and_process_unlink(sessfd, buf, offset+size);
			break;
		}  
		case GETDIRENTRIES:{
			deserilize_and_process_direntries(sessfd, buf, offset+size);
			break;
		}      
	}
}

int main(int argc, char**argv) {
	char *msg="Hello from server";
	//char buf[MAXMSGLEN+1];
	char *serverport;
	unsigned short port;
	int sockfd, sessfd, rv, i;
	struct sockaddr_in srv, cli;
	socklen_t sa_size;
	
	// Get environment variable indicating the port of the server
	serverport = getenv("serverport15440");
	if (serverport) port = (unsigned short)atoi(serverport);
	else port=15440;
	
	// Create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);	// TCP/IP socket
	if (sockfd<0) err(1, 0);			// in case of error
	
	// setup address structure to indicate server port
	memset(&srv, 0, sizeof(srv));			// clear it first
	srv.sin_family = AF_INET;			// IP family
	srv.sin_addr.s_addr = htonl(INADDR_ANY);	// don't care IP address
	srv.sin_port = htons(port);			// server port

	// bind to our port
	rv = bind(sockfd, (struct sockaddr*)&srv, sizeof(struct sockaddr));
	if (rv<0) err(1,0);
	
	// start listening for connections
	rv = listen(sockfd, 5);
	if (rv<0) err(1,0);

	while(1){	
	    // get messages and send replies to this client, until it goes away
	  
		sa_size = sizeof(struct sockaddr_in);
	    sessfd = accept(sockfd, (struct sockaddr *)&cli, &sa_size);
	    if (sessfd<0) err(1,0);
		// receive_and_process(sessfd);
		// //main server loop, handle clients
		rv = fork();
		if(rv == 0){
			close(sockfd);
		    while(1){
				receive_and_process(sessfd);
			}	
			close(sessfd);
			exit(0);
		}
		close(sessfd);
	}
	close(sockfd);
	return 0;
}
	    
       