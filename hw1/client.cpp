#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<iostream>
#include<string>
#include<vector>
#define MAXLINE 1500
using namespace std;

void run(FILE *fp, int sockfd);

int main(int argc, char** argv){
	int sockfd;
	int port;
	struct sockaddr_in servaddr;

	if(argc!=3){
		perror("usage: ./client <SERVER_IP> <SERVER_PORT>");
		exit(1);
	}
	port = atoi(argv[2]);
	
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	inet_pton(AF_INET,argv[1], &servaddr.sin_addr);

	if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))<0){
		perror("connect error\n");
		exit(1);
	}


	run(stdin, sockfd);

	return 0;
}

void run(FILE *fp, int sockfd){
	int max_num;
	fd_set rset;
	char sendline[MAXLINE], recvline[MAXLINE];
	bool eof = false;

	FD_ZERO(&rset);

	while(1){
		if(!eof)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		
		max_num = max(fileno(fp), sockfd)+1;

		select(max_num, &rset, NULL, NULL, NULL);

		if(FD_ISSET(sockfd, &rset)){ //socket is readable
			memset(recvline, 0, sizeof(recvline));//set recvline to 0
			int n = read(sockfd, recvline, MAXLINE);
			if(n<0){
				perror("error: read from server message\n");
				exit(1);
			}
			else if(n==0){
				if(eof==true)//normal termination
					return;
				else{
					perror("server terminated permaturely\n");
					exit(1);
				}
			}
			else
				fputs(recvline, stdout);
		}

		if(FD_ISSET(fileno(fp), &rset)){//input is readable
			if(fgets(sendline, MAXLINE, fp)==NULL){//received eof
				eof = true;
				shutdown(sockfd, SHUT_WR);
				FD_CLR(fileno(fp), &rset);
				continue;
			} 

			string str(sendline, 0, 4);

			if(str=="exit")
				return;

			write(sockfd, sendline, strlen(sendline));
		}
	}
}