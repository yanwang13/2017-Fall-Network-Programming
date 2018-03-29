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
#include<fstream>
#include"udp_header.hpp"
#define SERV_PORT 8020
using namespace std;

void run(int sockfd);

int main(int argc, char** argv){
	int sockfd, port;
	struct sockaddr_in servaddr;

	if(argc>2){
		cout << "usage: ./sender <port>\n";
		exit(0);
	}

	if(argc==2)
		port = atoi(argv[1]);
	else
		port = SERV_PORT;

	sockfd =  socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
	while(1){
		run(sockfd);
	}
	
	return 0;
}

void run(int sockfd){
	int n;
	sockaddr_in from;
	HEADER header;
	char packet[PACKET_SIZE]; //packet = header + data
	char ack_packet[PACKET_SIZE];
	char* data;
	int cur_offset = -1;
	ofstream output;
	cout << "prepare to read new file\n";
	while(1){
		socklen_t frlen = sizeof(from);

		//receive packets
		memset(packet, 0, sizeof(packet));
		if(n=recvfrom(sockfd, packet, sizeof(packet), 0, (struct sockaddr*) &from, &frlen)<0){
			cout << "recvfrom error\n";
			exit(1);
		}
		else{
			
			//deal with header
			memcpy(&header, packet, sizeof(HEADER));
			if(cur_offset == header.offset){//duplicate packet resend ack
				cout << "duplicated packet\n";
				memcpy(ack_packet, &header, sizeof(HEADER));
				sendto(sockfd, ack_packet, sizeof(ack_packet), 0,(struct sockaddr*) &from, frlen);
				continue;
			}
			else
				cur_offset = header.offset;
			
			//if(header.data_size != 1400)
				//packet[sizeof(HEADER)+header.data_size] = '\0';
			data = packet+sizeof(HEADER);
			cout<< "****************************************************\n";
			cout << "receive header >> " << "start: " << header.start << " offset: " << header.offset << endl;
			cout << "header.data_size: " << header.data_size << endl;
			cout << "data's length: " << strlen(data) <<endl;
			cout<< "****************************************************\n";
			//cout << "receive data : " << data <<endl;
			
			if(header.start){//receive filename
				output.open(data, ios::out|ios::binary);
				cout << "start to receive file. filename: " << data << endl;
			}

			else
				output.write(data, header.data_size);

			memcpy(ack_packet, &header, sizeof(HEADER));
			cout << "send ack packet >> " << "offset: " << header.offset <<endl;
			sendto(sockfd, ack_packet, sizeof(ack_packet), 0,(struct sockaddr*) &from, frlen);
			
			if(header.fin){//what if lost the final ack
				cout << "received fin\n";
				output.close();
				return;
			}
		}
	}
}
