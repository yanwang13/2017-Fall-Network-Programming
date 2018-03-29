#include <stdio.h>  
#include <stdlib.h>
#include <unistd.h>  
#include <vector>
#include <string>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <fstream>
#include <iostream>
#include <sstream>
#define MAXLINE 1450
#define MAXBUFSIZE 15000
#define RECV 1
#define SEND 2
using namespace std;

int connect_to_server();
void run();
//void GET(string filename, string size);
void GET(string filename);
void PUT(string filename);
void SLEEP(int sleep_t);
vector<string> preprocess(string input);

int port;
char* host_ip;
int cmdfd;//, putfd, loadfd;
int maxfd;
string username;
string client_id;
//int client_id;
int main(int argc, char** argv){

	int sockfd;
	string message;
	struct sockaddr_in servaddr;

	if(argc!=4){
		cout << "usage: ./client <host ip> <port> <username>\n";
		exit(1);
	}

	port = atoi(argv[2]);
	host_ip = argv[1];
	username = argv[3];

	maxfd = fileno(stdin);
	cmdfd = connect_to_server();

	message = "/username " + username;
	write(cmdfd, message.c_str(), message.length()); //send username

	run();
	return 0;
}

int connect_to_server(){
	struct sockaddr_in servaddr;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	inet_pton(AF_INET, host_ip, &servaddr.sin_addr);
	
	if(connect(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr))==-1)
		cout << "connection error\n";
	//else
		//cout << "connect to server\n";
	if(sockfd>=maxfd)//update maxfd
		maxfd = sockfd+1;
	return sockfd;
}

void run(){
	fd_set rset, wset;
	char sendline[MAXLINE], recvline[MAXLINE];
	bool eof = false;

	//FD_ZERO(&rset);
	//FD_ZERO(&wset);

	while(1){
		FD_ZERO(&rset);
		if(!eof)
			FD_SET(fileno(stdin), &rset);
		FD_SET(cmdfd, &rset);
		/*for(int i=0;i<trans.size();++i){
			if(trans[i].type==RECV)
				FD_SET(trans[i].socket, &rset);
			else
				FD_SET(trans[i].socket, &wset);
		}*/

		//select(maxfd, &rset, &wset, NULL, NULL);
		if(select(maxfd, &rset, NULL, NULL, NULL)<0){
			cout << "ERROR: select failed\n";
			if(errno==EBADF)
				cout << "invalid discripter";
		}

		if(FD_ISSET(cmdfd, &rset)){//server response
			memset(recvline, 0, MAXLINE);
			int n = read(cmdfd, recvline, MAXLINE);

			if(n<0){
				cout << "ERROR: read from server message\n";
				exit(1);
			}
			else if(n==0){
				if(eof)
					return; //normal termination
				else{
					cout << "server terminated permaturely\n";
					exit(1);
				}
			}
			else{
				vector<string> response = preprocess(recvline);
				//fputs(recvline, stdout); //print out the server message
				/*if(response[0]=="GET" && response.size()==2)//GET filename file_size
					GET(response[1]);
					//GET(response[1], response[2]);
					//GET(response[1], atoi(response[2].c_str()));*/
				while(!response.empty()){
					if(response[0]=="GET"){
						//while(!response.empty()){
						GET(response[1]);
						//}
					}
					/*else if(response[0]=="PUT"){
						PUT(response[1]);
					}*/
					else if(response[0]=="Welcome"){
						//client_id = atoi(response[1].c_str());
						client_id = response[1];
						//cout << "client_id: " << client_id;
						cout << "Welcome to the dropbox-like server! : " + username  + "\n";
					}
					response.erase(response.begin(), response.begin()+2);
				}
				
			}
		}

		if(FD_ISSET(fileno(stdin), &rset)){
			memset(sendline, 0, MAXLINE);
			if(fgets(sendline, MAXLINE, stdin)==NULL){//received eof
				eof = true;
				shutdown(cmdfd, SHUT_WR);//send FIN, close one side of socket. still able to receive data
				FD_CLR(fileno(stdin), &rset);
			}

			vector<string> cmd = preprocess(sendline);
			/*cout << "client's input: ";
			for(int i=0;i<cmd.size();++i)
				cout << cmd[i] << " ";
			cout << endl;*/

			//write(sockfd, sendline, strlen(sendline));


			if(cmd[0]=="/exit"){
				string message = "/exit " + username;
				write(cmdfd, sendline, strlen(sendline));
				close(cmdfd);
				return;
			}
			/*else if(cmd[0]=="/put"){
				string message = "PUT " + cmd[1];
				write(cmdfd, message.c_str(), message.length());
			}*/
			else if(cmd[0]=="/put"){
				PUT(cmd[1]);
			}
			else if(cmd[0]=="/sleep" && cmd.size()==2)
				SLEEP(atoi(cmd[1].c_str()));
			else
				cout << "wrong command\n";
			
		}

		/*for(int i=0;i<trans.size();++i){
			if(trans.type==RECV && FD_ISSET(trans[i].socket, &rset)){

			}
			if(trans.type==SEND && FD_ISSET(trans[i].socket, &wset)){

			}
		}*/

	}
}

//void GET(string filename, string size){
void GET(string filename){
	int fd;
	fstream file(filename, ios::out | ios::binary);
	char buffer[MAXLINE];
	if(!file){
		cout << "fail to open " << filename << " at client\n";
		return;
	}
	//transfer_info get_file(fd, filename, RECV, size);
	//trans.push_back(get_file);
	//string message = "/get " + filename + " " + size + " " + client_id;// + " " + username;//+ " " + id;
	fd = connect_to_server();
	string message = "/get " + filename + " " + username;
	write(fd, message.c_str(), message.length());
	//cout << "message: " << message << endl;
	cout << "Downloading file : " << filename << endl;
	cout << "Progress: [";
	
	memset(buffer, 0, sizeof(buffer));
	int n;
	while((n=read(fd, buffer, MAXLINE))!=0){
		if(n>0){
			//cout <<"read: " << n << endl;
			file.write(buffer, n);
			//fputs(buffer, stdout);
			bzero(buffer, sizeof(buffer));
			cout << "#";
		}
		
	}

	cout << "]\n";
	file.close();
	close(fd);
	//shutdown(fd, SHUT_WR);
	cout << "Download " << filename << " complete!\n";
	return;
}
void PUT(string filename){// /put filename size username client_id
	int putfd;
	string message;
	ifstream file(filename, ios::in | ios::binary);
	char buffer[MAXLINE];
	long size;
	int segment;
	int sent = 0;
	

	if(!file){
		cout << "fail to open " << filename << " at client\n";
		return;
	}

	file.seekg(0, file.end);
	size = file.tellg();
	file.seekg(0);

	//message = "/put " + filename + " " + to_string(size) + " " + username + " " + client_id;
	putfd = connect_to_server();
	message = "/put " + filename + " " + username + " " + client_id;
	write(putfd, message.c_str(), message.length());
	int n;
	while((n=read(putfd, buffer, sizeof(buffer)))<0);
	//cout << "buffer: " << buffer << endl;
	cout << "Uploading file : " << filename << endl;

	//buffer = new char[size];
	//segment = size/20;
	cout << "Progress: [";

	/*while(!file.eof()){
		memset(buffer, 0, sizeof(buffer));
		file.read(buffer, sizeof(buffer));
		write(putfd, buffer, sizeof(buffer));
		fputs(buffer, stdout);
		cout << "#";
	}*/
	int nread;
	while(sent!=size){
		nread = (size-sent)< MAXLINE ? size-sent : MAXLINE;
		memset(buffer, 0, sizeof(buffer));
		file.read(buffer, nread);
		write(putfd, buffer, nread);
		cout << "#";
		//fputs(buffer, stdout);
		sent += nread;
	}
	//file.read(buffer, size);
	//write(putfd, buffer, size);
	/*while(!file.eof()){
		memset(buffer, 0, sizeof(buffer));
		file.read(buffer, sizeof(buffer));
		write(putfd, buffer, sizeof(buffer));
	}*/
	cout << "]\n";
	//fputs(buffer, stdout); //print on screen

	file.close();
	//shutdown(putfd, SHUT_WR);
	close(putfd);
	cout << "Upload " << filename << " complete!\n";

	return;
}


void SLEEP(int sleep_t){
	cout << "Client starts to sleep\n";
	for(int i=1;i<=sleep_t;++i){
		sleep(1);
		cout << "Sleep " << i << "\n";
	}
	cout << "Client wakes up\n";
	return;
}

vector<string> preprocess(string input){
	stringstream ss;
	string str;
	vector<string> request;

	ss << input;

	while(ss>>str) request.push_back(str);

	return request;
}