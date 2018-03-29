#include<errno.h>
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
#include<fstream>
#include<vector>
#include<sstream>
#include<signal.h>
#include"udp_header.hpp"
using namespace std;

static void handler(int signo);
void run(char* filename, int sockfd);
bool send_packet(int sockfd, HEADER &header, char* data);

int main(int argc, char** argv){
	int sockfd, port;
	struct sockaddr_in servaddr;
	

	if(argc!=4){
		cout << "usage: ./sender <receiver IP><receiver port><file name>\n";
		exit(0);
	}
	port = atoi(argv[2]);

	//initialize socket
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));


	run(argv[3], sockfd);
	return 0;
}

void run(char* filename, int sockfd){
	int n;
	char send_buf[BUFFER_SIZE];//store extracted file content
	char packet[PACKET_SIZE]; //packet = header + send_buf
	ifstream input(filename, ios::in|ios::binary);
	HEADER header;
	
	signal(SIGALRM, handler);
	siginterrupt(SIGALRM, 1);

	//fisrt packet for filename
	header.start = true;
	header.fin = false;
	header.offset = 0;
	header.data_size = 0;
	
	while(!send_packet(sockfd, header, filename));//send file name

	while(1){
		memset(send_buf, 0, sizeof(send_buf));
		input.read(send_buf, sizeof(send_buf));

		header.start = false;
		header.fin = false;
		header.offset = input.tellg();
		header.data_size = input.gcount();

		if(input.eof())
			break;

		while(!send_packet(sockfd, header, send_buf));

	}

	header.fin = true;
	header.offset = -1;
	header.data_size = input.gcount();
	while(!send_packet(sockfd, header, send_buf)); //last packet

	cout << "finish file transfer. prepare to close socket\n";
	input.close();
	close(sockfd);
	return;
}


bool send_packet(int sockfd, HEADER &header, char* data){
	char recv_ack[PACKET_SIZE];
	int n;
	char packet[PACKET_SIZE];
	
	//maket packet
	memcpy(packet, &header, sizeof(HEADER));
	cout << "header info >> " << "start: " << header.start << "  fin: " << header.fin << endl;
	cout << "data offset: " << header.offset << endl;
	cout << "data size: " << header.data_size << endl;
	cout << "data length: " << strlen(data) << endl;

	if(header.start)
		memcpy(packet+sizeof(HEADER), data, strlen(data));
	else
		memcpy(packet+sizeof(HEADER), data, header.data_size);
	write(sockfd, packet, sizeof(packet));

	alarm(1);

	if((n = read(sockfd, recv_ack, PACKET_SIZE))<0){
		if(errno==EINTR){
			cout << "packet time out\n";
			return false;
		}
		else{
			perror("recvfrom error\n");
			exit(1);
		}
	}
	else{
		alarm(0);
		cout << "receive packet\n\n";
		return true;
	}
}
static void handler(int signo){
	return; //just interrupt from recvfrom
}
