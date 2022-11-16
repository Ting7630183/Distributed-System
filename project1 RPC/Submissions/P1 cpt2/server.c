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
#define FD_OFFSET 7000
#define MAXMSGLEN 1024

int main(int argc, char**argv) {
	//printf("enter the server\n");
	char *msg="Hello from server";
	char buf[MAXMSGLEN+1];
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
	    //printf("I don't know why, please tell me\n");
	    // for(i = 0; i < 10; i++){
			sa_size = sizeof(struct sockaddr_in);
	        sessfd = accept(sockfd, (struct sockaddr *)&cli, &sa_size);
	        if (sessfd<0) err(1,0);
		    // main server loop, handle clients
	    
            int size = sizeof(int);
	        int total_size;
	        int total_receive = 0;
	        while((rv =recv(sessfd, buf, size-total_receive, 0)) > 0){
		        //printf("enter the first while loop\n");
		        total_receive += rv;
		        if(total_receive >= size){
			        break;
		        }
	        }
	        //printf("total_receive in the first while loop is: %d\n", total_receive);
	        int offset = 0;
	        memcpy(&total_size, buf+offset, size);
	        //printf("total size is: %d\n", total_size);

	        while((rv=recv(sessfd, buf+total_receive, total_size - total_receive, 0)) > 0){
		        total_receive += rv;
		        if(total_receive >= total_size){
			        break;
		        }	
	        }
	        //printf("total_receive in the second while loop is: %d\n", total_receive);

	        int upcode;
	        offset += size;
	        size = sizeof(int);
	        memcpy(&upcode, buf+offset, size);
	        //printf("upcode is: %d\n", upcode);
		
	        switch(upcode) {
		        default:{
				    break;
		        }
	        case 1:{
		        mode_t m;
		        offset += size;
		        size = sizeof(mode_t);
		        memcpy(&m, buf+offset, size);

		        int flags;
		        offset += size;
		        size = sizeof(int);
		        memcpy(&flags, buf+offset, size);

		        int size_of_pathname;
		        offset += size;
		        size = sizeof(int);
		        memcpy(&size_of_pathname, buf+offset, size);
		        //printf("size of the pathname is: %d\n", size_of_pathname);

		        char pathname[size_of_pathname+1];
		        offset += size;
		        size = size_of_pathname+1;
		        memcpy(&pathname, buf+offset, size);
		        //printf("the pathname is: %s\n", pathname);

		        //printf("flags in the open is: %d\n", flags);
		        int open_fd = open(pathname, flags, m) + FD_OFFSET;
		        //printf("open_fd in the server side is: %d\n", open_fd);

		        int total_size_out = sizeof(int) + sizeof(int) + sizeof(int);
		        //printf("total size out is: %d\n", total_size_out);
		        char *buffer = malloc(total_size_out);
        
		        int offset_out = 0;
		        int size_out = sizeof(int);
		        memcpy(buffer+offset_out, &total_size_out, size_out);
		        //printf("the total size out in open is:%d\n", total_size_out);

		        offset_out += size_out;
		        memcpy(buffer+offset_out, &open_fd, size_out);

		        offset_out += size_out;
		        //printf("the errno in the server open is.............:%d\n", errno);
		        int err = errno;
		        memcpy(buffer+offset_out, &err, size_out);
		        send(sessfd, buffer, total_size_out+1, 0); 
		        break;
		        }		   
	        case 2: {
		        int write_fd;
		        offset += size;
		        size = sizeof(int);
		        memcpy(&write_fd, buf+offset, size);
		        write_fd -= FD_OFFSET;
		        //printf("write_fd is: %d\n", write_fd);

		        int count;
		        offset += size;
		        memcpy(&count, buf+offset, size);
		        //printf("count of write is: %d\n", count);

		        char *buffer_write = malloc(count);
		        offset += size;
		        memcpy(buffer_write, buf+offset, count);
		        //printf("the content to write is: %s\n", buffer_write);

		        ssize_t write_return = write(write_fd, buffer_write, count);
		        //printf("the return value of the write is: %ld\n", write_return);

		        int total_size_out = sizeof(ssize_t) + sizeof(int);
		        char *buffer = malloc(total_size_out);
		    
		        int offset_out = 0;
		        int size_out = sizeof(ssize_t);
		        memcpy(buffer+offset_out, &write_return, size_out);

		        offset_out += size_out;
		        //printf("the errno in the werver write is.............:%d\n", errno);
		        size_out = sizeof(int);
		        int err = errno;
		        memcpy(buffer+offset_out, &err, size_out);
		        send(sessfd, buffer, total_size_out+1, 0); 
		        break;
	        }
	        case 3:{
		        int close_fd;
		        offset += size;
		        size = sizeof(int);
		        memcpy(&close_fd, buf+offset, size);
		        close_fd -= FD_OFFSET;
		        //printf("close_fd on the server side is: %d\n", close_fd);
		        int close_result = close(close_fd);

		        int total_size_out = sizeof(int) +  sizeof(int);
		        char *buffer = malloc(total_size_out);
        
		        int offset_out = 0;
		        int size_out = sizeof(int);
		        memcpy(buffer+offset_out, &close_result, size_out);

		        offset_out += size_out;
		        int err = errno;
		        //printf("the errno in server close is.............:%d\n", err);
		        memcpy(buffer+offset_out, &err, size_out);
		        send(sessfd, buffer, total_size_out+1, 0); 
		        break;
	        }	         
	        
	    }
		// either client closed connection, or error
		if (rv<0) err(1,0);
	    close(sessfd);
				
	}
	
	close(sockfd);
	return 0;
}

