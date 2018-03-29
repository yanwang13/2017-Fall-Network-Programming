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
#include <sys/ipc.h>
#include <algorithm>
#include <cstring>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>

using namespace std;

int main(int argc, char** argv){
	int sockfd;

	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0){
		cout << "socket error\n";
		exit(1);
	}

	struct ifconf ifc;
	struct ifreq *ifr;
	char buf[5000];
	//char str[1500];

	ifc.ifc_len = 5000;
	ifc.ifc_buf = buf;
	ifr = (struct ifreq*) buf;
	setuid(getuid());
	if(ioctl(sockfd, SIOCGIFCONF, &ifc)<0){
		cout << "ERROR: ioctl()\n";
		exit(1);
	}

	struct sockaddr *sa;
	struct sockaddr_in addr;
	if(argc==3){
		addr.sin_family = AF_INET;
		inet_pton(AF_INET, argv[2], &addr.sin_addr);
		//ifr->ifr_addr = *((sockaddr*)addr);
		sa = &(ifr->ifr_addr);
		((struct sockaddr_in*)sa)->sin_addr = addr.sin_addr;

		//inet_pton(AF_INET, argv[1], &(ifr->ifr_addr));//needs sockaddr struct 
		ioctl(sockfd, SIOCSIFADDR, ifr);//set the address

		//do ifconfig 2nd
		memset(buf, 0, sizeof(buf));
		ioctl(sockfd, SIOCGIFCONF, &ifc);
		ioctl(sockfd, SIOCGIFADDR, ifr);
		sa = &(ifr->ifr_addr);
		cout << "lo->inet addr: " << inet_ntoa(((sockaddr_in*)sa)->sin_addr) <<endl;
	}
	else{
		ioctl(sockfd, SIOCGIFADDR, ifr);
		sa = &(ifr->ifr_addr);
		//inet_ntop(AF_INET, (((struct sockaddr_in*)sa)->sin_addr), str, sizeof(str));
		//inet_ntop(AF_INET, &(ifr->ifr_addr), str, sizeof(str));
		cout << "lo->inet addr: " << inet_ntoa(((sockaddr_in*)sa)->sin_addr) <<endl;
		//cout << str << endl;
		//cout << "lo->inet addr: " << inet_ntoa(ifr->ifr_addr) << endl;
	}

	return 0;
}

                  