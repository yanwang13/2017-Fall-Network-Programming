#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <netinet/in_systm.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <sys/ipc.h>
#include <algorithm>
#include <cstring>
#include <sys/time.h>
#include <fcntl.h>
#include <netdb.h>
#define ALARM_TIME 1
#define MAX_BUF_SIZE 2048

#define ICMP_DATA_SIZE 56
#define ICMP_HEADER_SIZE 8
#define MAX_PING_IP 254

using namespace std;

struct PingData
{
	sockaddr_in addr;
	bool send;
	bool recv;
	int pingnum;
	timeval sendtime;
	PingData(){
		send = false;
		recv = false;
	}
	
};

sockaddr_in addr_send, addr_recv;
socklen_t sendaddr_len, recvaddr_len;
int fd;
int nextIP;
PingData* pingData;
sockaddr_in curping_addr;
int curping;

char sendbuf[MAX_BUF_SIZE];
char recvbuf[MAX_BUF_SIZE];
char ipBuf[MAX_BUF_SIZE];
const char* subnet;

void tv_sub(timeval& lhs, const timeval& rhs);
void sendICMP(int signo);
void recvICMP();
void main_loop();
void bufclear(char* buf);
void fillHostname(sockaddr_in& addr, char** dest);
bool checkTimeout();
unsigned short in_cksum(unsigned short* addr, int len);

int main(int argc, char** argv){
	if(argc<2){
		cout << "usage: pingsubnet <subnet IP>" << endl;
		exit(1);
	}

	subnet = argv[1];
	nextIP = 1;
	pingData = new PingData[MAX_PING_IP+1];
	curping = 0;

	signal(SIGALRM, sendICMP);
	siginterrupt(SIGALRM, 1);

	fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setuid(getuid());

	if(fd<0){
		cout << "cannot create socket\n";
		exit(1);
	}

	alarm(1);
	main_loop();
	delete [] pingData;
	close(fd);

	return 0;
}

void main_loop(){
	while(true){
		recvICMP();
		if(!checkTimeout()){
			break;
		}
	}
}

bool checkTimeout(){
	bool allFinish = true;
	timeval tv;
	char* hostname;
	char ipBuf[MAX_BUF_SIZE];

	//for(int i=1;i<=3MAX_PING_IP;++i){
	for(int i=1;i<=MAX_PING_IP;++i){
		auto& entry = pingData[i];
		if(entry.send){
			if(!entry.recv){
				allFinish = false;

				gettimeofday(&tv, NULL);
				tv_sub(tv, entry.sendtime);
				if(tv.tv_sec>=3){
					//cout << "timeout ping packet for " << i << endl;
					entry.recv=true;
					fillHostname(entry.addr, &hostname);
					bufclear(ipBuf);
					inet_ntop(AF_INET, &entry.addr.sin_addr, ipBuf, MAX_BUF_SIZE);
					/*if(entry.pingnum==0){

						printf("%s", ipBuf);
						if(hostname){
							printf("(%s)", hostname);
						}
					}
					printf(" RTT%d=* ", entry.pingnum+1);
					if(entry.pingnum==2)
						cout << endl;*/
					/** add something here!!!**/
					/** add something here!!!**/
					/** add something here!!!**/
					/** add something here!!!**/
				}
			}
			else{
				//cout << "IP " << i << "received\n";
			}

		}
		else{
			allFinish = false;
		}

	}
	return !allFinish;
}

void recvICMP(){
	int read_n;
	ip* ipData;
	icmp* icmpData;
	size_t ipHeaderLen;
	size_t icmpLen;
	timeval tvRecv;
	double rtt;

	char* hostname = NULL;

	recvaddr_len = sizeof(addr_recv);
	bufclear(recvbuf);

	read_n = recvfrom(fd, recvbuf, MAX_BUF_SIZE, 0, (sockaddr*)&addr_recv, &recvaddr_len);
	if(read_n<0){
		if(errno!=EWOULDBLOCK && errno!=EINTR){
			cout << "unknown error when receive" << endl;
		}
		//if(errno==EINTR)
			//cout << "socket EINTR\n";
		if(errno==EWOULDBLOCK){
			//cout << "socket timeout\n";
			if(nextIP >= MAX_PING_IP && curping==2)
				alarm(0);
			else
				alarm(1); //socket time out send next ping

			if(curping==1){//first ping
				inet_ntop(AF_INET, &curping_addr.sin_addr, ipBuf, MAX_BUF_SIZE);
				cout << ipBuf;
				fillHostname(curping_addr, &hostname);
				if(hostname){
					printf("(%s)", hostname);
				}
			}

			if(curping==0)
				printf("RTT3=*");
			else
				printf("RTT%d=*", curping);

			if(curping==0)
				cout << endl;
			else
				cout << ", " << flush;
		}
		return;
	}

	if(nextIP >= MAX_PING_IP && curping==2)
		alarm(0);
	else
		alarm(1);


	ipData = (ip*) recvbuf;

	if(ipData->ip_p != IPPROTO_ICMP){
		cout << "Protocol is not ICMP\n";
		return;
	}
	ipHeaderLen = (ipData->ip_hl) << 2;
	if((icmpLen= read_n-ipHeaderLen)<ICMP_HEADER_SIZE){
		cout << "ICMP packet header size errn" << endl;
		return;
	}

	icmpData = (icmp*)(recvbuf+ipHeaderLen);
	if(icmpData ->icmp_type != ICMP_ECHOREPLY){
		return;
	}
	if(icmpLen < ICMP_HEADER_SIZE+sizeof(timeval)){
		cout << "ICMP packet data lost!" << endl;
		return;
	}

	//extract data
	timeval& tvSend = *(timeval*)icmpData->icmp_data;
	gettimeofday(&tvRecv, NULL);
	tv_sub(tvRecv, tvSend);
	rtt = tvRecv.tv_sec*1000.0 + tvRecv.tv_usec/1000.0;

	//extract IP
	inet_ntop(AF_INET, &addr_recv.sin_addr, ipBuf, MAX_BUF_SIZE);
	if(curping==1){
		cout << ipBuf;
		fillHostname(addr_recv, &hostname);
		if(hostname){
			printf("(%s)", hostname);
		}
	}
/**** change something ***********************************/
	/**** change something ***********************************/
	/**** change something ***********************************/
	if(curping==0)
		printf("RTT3=%.3fms", rtt);
	else
		printf("RTT%d=%.3fms", curping, rtt);

	if(curping==0)
		cout << endl;
	else
		cout << ", " << flush;
	//printf(" RTT=%.3fms\n", rtt);

	char* lastNumBuf = ipBuf + strlen(subnet)+1;
	int lastNum = atoi(lastNumBuf);
	pingData[lastNum].recv = true;
}

void sendICMP(int signo){
	char sendIP[MAX_BUF_SIZE];
	timeval tvSend;
	icmp* icmpData;
	int len = ICMP_HEADER_SIZE + ICMP_DATA_SIZE;
	/*if(nextIP >= MAX_PING_IP){
		// no alarm again
		alarm(0);
	}
	else{
		//set alarm again
		alarm(ALARM_TIME);
	}*/

	bufclear(sendIP);
	snprintf(sendIP, MAX_BUF_SIZE, "%s.%d", subnet, nextIP);//sendIP string

	addr_send.sin_family = AF_INET;
	inet_pton(AF_INET, sendIP, &addr_send.sin_addr);
	sendaddr_len = sizeof(addr_send);

	gettimeofday(&tvSend, NULL);
	pingData[nextIP].send = true;
	pingData[nextIP].addr = addr_send;
	pingData[nextIP].sendtime = tvSend;
	curping_addr = addr_send;

	icmpData = (icmp*)sendbuf;
	icmpData->icmp_type = ICMP_ECHO;
	icmpData->icmp_code = 0;
	icmpData->icmp_id = 0;
	icmpData->icmp_seq = 0;
	memset(icmpData->icmp_data, 0xa5, ICMP_DATA_SIZE);
	memcpy(icmpData->icmp_data, &tvSend, sizeof(tvSend));
	icmpData->icmp_cksum = 0;
	icmpData->icmp_cksum = in_cksum((unsigned short*)icmpData, len);

	sendto(fd, sendbuf, len, 0, (sockaddr*)&addr_send, sendaddr_len);
/////////////////////////////////////////////////////
	//send three packets
///////////////////////////////////////////////////////
	//cout << "send IP:" << sendIP <<endl;
	//cout << "send ICMP> curping = " << curping << endl;
	if(curping<2){
		++curping;
	}
	else{
		curping = 0;
		++nextIP;
	}	
}

void bufclear(char* buf){
	bzero(buf, MAX_BUF_SIZE);
}


unsigned short in_cksum(unsigned short* addr, int len){
	int nleft = len;
	int sum = 0;
	unsigned short* w = addr;
	unsigned short answer = 0;

	while(nleft>1){
		sum += *w++;
		nleft-=2;
	}

	if(nleft==1){
		*(unsigned char*)(&answer) = *(unsigned char*)w;
		sum += answer;
	}

	sum = (sum>>16) + (sum & 0xffff);
	sum += (sum>>16);
	answer = ~sum;
	return answer;
}

void tv_sub(timeval& lhs, const timeval& rhs){
	if((lhs.tv_usec-=rhs.tv_usec)<0){
		lhs.tv_usec += 1000000;
		lhs.tv_sec--;
	}
	lhs.tv_sec -= rhs.tv_sec;
}

void fillHostname(sockaddr_in& addr, char** dest){
	auto pHost = gethostbyaddr(&addr.sin_addr, sizeof(addr.sin_addr), AF_INET);
	if(pHost){
		*dest = pHost->h_name;
	}
	else{
		*dest = NULL;
	}
}