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
#include <sys/stat.h>
#include <netinet/in.h>  
#include <fstream>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#define LISTENQ 100
#define MAXLINE 1450
#define MAXBUFSIZE 15000
#define RECV 1
#define SEND 2
using namespace std;

class USER{
	public:
		//vector<CLIENT> clients;
		vector<int> clients;
		string username;
		vector<string> filelist;
};

int maxfd;
int listenfd;
vector<USER> users;
//vector<transfer_info> trans;
vector<int> fds;//save undefined fds and cmdfds
fd_set rset, wset, allset;
char read_buf[MAXBUFSIZE], write_buf[MAXBUFSIZE];

void check_user();
void run();
void close_socket(int fd);
void new_conncection();
void add_client(string name, int fd);
vector<string> preprocess(string input);
void PUT(int sockfd, string filename, string name, int client_id);
//void PUT(int sockfd, string filename,int size, string name, int client_id);
void GET(int sockfd, string filename, string name);
//void GET(int sockfd, string filename, int size, string name);
void sync_file(USER &user, int sockfd);
void sync_client(USER &user, int no_send, string filename);
void write_n(int sockfd, char* buf, int target);

/*class transfer_info{
	public:
		int socket;
		fstream file;
		int file_size;
		int cur;
		string username;
		int type;//receive or send
		transfer_info(int socket, stinrg filename, int type, int file_size, string username):socket(socket), file_size(file_size), username(username){
			if(type==RECV)
				file.open(filename, ios::out|ios::binary);
			else
				file.open(filename, ios::in|ios::binary);
			finish = false;
			cur = 0;
		}

}*/


/*class CLIENT{
	public:
		int cmdfd;
		vector<stirng> filelist;
		
};*/


int main(int argc, char** argv){
	//int listenfd;
	socklen_t len;
	struct sockaddr_in servaddr;

	if(argc!=2){
		cout << "usage: ./server <port>\n";
		exit(1);
	}
	int port = atoi(argv[1]);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr));

	listen(listenfd, LISTENQ);

	maxfd = listenfd +1;
	//cout << "check!\n";
	//check_user();
	run();
	return 0;
}

void run(){
	int eof;
	//struct sockaddr_in cliaddr;
	//socklen_t clilen;
	//ssize_t n, nwritten;
	//fd_set rset, wset;
	int n;
	string message;
	char command[MAXLINE];
	//FD_ZERO(&rset);
	
	while(1){
		
		FD_ZERO(&rset);

		//setting rset
		FD_SET(listenfd, &rset);
		//cout << "fd set: ";
		for(int d=0;d<fds.size();++d){
			FD_SET(fds[d], &rset);
			//cout << fds[d] << " ";
		}
		//cout << endl;
		/*for(int i=0;i<trans.size();++i){
			if(trans[i].type==RECV)
				FD_SET(trans[i].socket, &rset);
			else
				FD_SET(trans[i].socket, &wset);
		}*/

		//select(maxfd, &rset, NULL, NULL, NULL);
		if(select(maxfd, &rset, NULL, NULL, NULL)==-1){
			cout << "ERROR: select failed\n";
			if(errno==EBADF)
				cout << "invalid discripter";
			else if(errno==EINTR)
				cout << "signal interrupt";
			else if(errno==EINVAL)
				cout << "maxfd or timeout is invalid";
			else if(errno==ENOMEM)
				cout << "no more memory";
			cout << endl;
		}
		if(FD_ISSET(listenfd, &rset)){//new connection
			//cout << "listenfd is set\n";
			new_conncection();
			FD_CLR(listenfd, &rset);
			//if(FD_ISSET(listenfd, &rset))
				//cout << "listenfd is set\n";
		}
		for(int fd=3;fd<maxfd;++fd){
			
			if(FD_ISSET(fd, &rset)){
				memset(command, 0, sizeof(command));
				if((n=read(fd, command, MAXLINE))<0){
					if(errno!=EWOULDBLOCK)
						cout << "ERROR: read from socket\n";
				}
				else if(n==0){
					//socket closed!??
					//cout << "socket closed prematurely\n";
					
					close_socket(fd);			
				}
				else{
					vector<string> cmd = preprocess(command);
					cout << "client command: " << command << endl;
					if(cmd[0]=="/username"){
						add_client(cmd[1], fd);
					}
					/*else if(cmd[0]=="PUT"){
						string message = "PUT " + cmd[1];
						write(fd, message.c_str(), message.length());
					}*/
					else if(cmd[0]=="/put"){//put filename file_size username client_id
						//ready to read file from client
						PUT(fd, cmd[1], cmd[2], atoi(cmd[3].c_str()));
						//PUT(fds[i], cmd[1], atoi(cmd[2]), cmd[3], atoi(cmd[4]));//change fd's type
						//fds.erase(fds.begin()+i);
					}
					else if(cmd[0]=="/get"){// /get filename username
						GET(fd, cmd[1], cmd[2]);
						//GET(fds[i], cmd[1], atoi(cmd[2]), cmd[3]);// /get filename file_size id
						//fds.erase(fds.begin()+i);
					}
					else if(cmd[0]=="/exit"){
						//shutdown the socket
						/*close(fd);
						int uu;
						for(uu=0;uu<users.size();++uu){
							if(users[uu].username == cmd[1])
								break;
						}
						int k;
						for(k=0;k<users[uu].clients.size();++k){
							if(users[uu].clients[k]==fd)//find the client
								break;
						}
						if(k!=users[uu].clients.size())
							users[uu].clients.erase(users[uu].clients.begin()+k);*/
						close_socket(fd);
						//fds.erase(fds.begin()+i);
					}
					else{
						cout << "wrong command\n";
					}
				}
			}
		}
	}

}
void check_user(){
	for(int i=0;i<users.size();++i){
		cout << "USER: " << users[i].username << " > ";
		for(int j=0;j<users[i].clients.size();++j){
			cout << users[i].clients[j] << " ";
		}
		cout << endl;
		
		for(int j=0;j<users[i].filelist.size();++j)
			cout << users[i].filelist[j] << " ";
		cout << endl;
	}
}
void close_socket(int fd){
	int k;
	//cout << "close socket\n";
	for(k=0;k<fds.size();++k){
		if(fds[k]==fd)
			break;
	}
	fds.erase(fds.begin()+k);
	
	int i, j;
	for(i=0;i<users.size();++i){
		for(j=0;j<users[i].clients.size();++j){
			if(users[i].clients[j]==fd)
				break;
		}
	}
	if((i!=users.size())&&(j!=users[i].clients.size()))
		users[i].clients.erase(users[i].clients.begin()+j);
	close(fd);
	return;
}

void new_conncection(){
	cout << "new connection\n";
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	int connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
	//set_nonblock
	int val = fcntl(connfd, F_GETFL, 0);
	fcntl(connfd, F_SETFL, val | O_NONBLOCK);

	fds.push_back(connfd);
	//cout << "current fd count: " << fds.size()<<endl;

	if(connfd>=maxfd)
		maxfd = connfd+1;
	return;
}

void add_client(string name, int fd){
	//CLIENT new_client;
	//new_client.cmdfd = fd;

	int i;
	for(i=0;i<users.size();++i){
		if(users[i].username==name){
			users[i].clients.push_back(fd);
			break;
		}
	}
	if(i==users.size()){
		USER new_user;
		new_user.username = name;
		users.push_back(new_user);
		users[i].clients.push_back(fd);
		if(mkdir(name.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)==-1)
			cout << "make directory failed\n";
		else
			cout << "make new directory: " << name <<endl;
	}
	string message = "Welcome " + to_string(fd) + "\n";
	write(fd, message.c_str(), message.length());

	//sync_file(users[i], new_client);
	sync_file(users[i], fd);
}

vector<string> preprocess(string input){
	stringstream ss;
	string str;
	vector<string> request;

	ss << input;

	while(ss>>str) request.push_back(str);

	return request;
}
//void PUT(int sockfd, string filename,int size, string name, int client_id){
void PUT(int sockfd, string filename, string name, int client_id){
	//transfer_info put_file(sockfd, filename, RECV, size, name);
	//trans.push_back(put_file);
	cout << "PUT function\n";
	string message = "PUT\n";
	write(sockfd, message.c_str(), message.length());
	fstream file;
	char buffer[MAXLINE];
	int k, n;
	for(k=0;k<users.size();++k){
		if(users[k].username == name){//find the username and add filename
			users[k].filelist.push_back(filename);
			break;
		}
	}
	if(k==users.size())
		cout << "cannot find the user name\n";
	//users[uid].filelist.push_back(filename);
	//FD_SET(sockfd, &rset);
	string filepath = name + "/" + filename;
	file.open(filepath, ios::out | ios::binary);
	cout << "filepath: " << filepath << endl;
	if(!file){
		cout << "cannot open file: " << filepath << endl;
	}

	while((n=read(sockfd, buffer, MAXLINE))!=0){
		if(n<0){
			if(errno==EWOULDBLOCK)
				continue;
				//cout << "read WOULDBLOCK! ";
		}
		else{
			file.write(buffer, n);
			bzero(buffer, MAXLINE);
		}
		
	}
	if(n==0)
		cout << "socket receive FIN\n";
	file.close();
	//close(sockfd);
	close_socket(sockfd);

	sync_client(users[k], client_id, filename);
	return;
}

void GET(int sockfd, string filename, string name){// /get filename username
	cout << "GET function\n";
	fstream file;
	char buffer[MAXLINE];
	long size;
	int sent = 0;
	int nread;
	string filepath = name + "/" + filename;
	file.open(filepath, ios::in | ios::binary);
	if(!file){
		cout << "cannot open file: " << filepath << endl;
	}
	
	file.seekg(0, file.end);
	size = file.tellg();
	file.seekg(0);

	while(sent!=size){
		nread = (size-sent)< MAXLINE ? size-sent : MAXLINE;
		memset(buffer, 0, sizeof(buffer));
		file.read(buffer, nread);
		write_n(sockfd, buffer, nread);
		//fputs(buffer, stdout);
		sent += nread;
	}
	
	file.close();
	//shutdown(sockfd, SHUT_WR);
	close_socket(sockfd);
	return;
}

void write_n(int sockfd, char* buf, int target){
	//cout << "write_n function\n";
	int written = 0;
	int n = 0;
	while(written!=target){
		n=write(sockfd, buf+written, target-written);
		if(n<0 && errno!=EWOULDBLOCK){
			cout << "ERROR: write to socket\n";
			return;
		}
		else if(n<0 && errno==EWOULDBLOCK)
			cout << "write WOULDBLOCK\n";
		written += n;
		//cout << "write: " << n << endl;
	}
	
}

void sync_file(USER &user, int sockfd){//sync file when client login
	string message;
	cout << "sync to log in client\n";
	cout << "filelist's size: " << user.filelist.size() << endl;
	for(int i=0;i<user.filelist.size();++i){
		//message = "GET " + user.filelist[i] + file_size + ; //GET filename file_size
		message = "GET " + user.filelist[i] + " ";
		write(sockfd, message.c_str(), message.length());
		cout << message << endl;
	}
}

void sync_client(USER &user, int no_send, string filename){//sync file to all the other client
	cout << "sync to other clients\n";
	string message = "GET " + filename + " ";
	for(int i=0;i<user.clients.size();++i){
		if(user.clients[i]==no_send)
			continue;
		else{
			if(write(user.clients[i], message.c_str(), message.length())<0){
				if(errno==EWOULDBLOCK)
					cout << "write to client will blocked\n";
			}
			else{
				cout << "write message to : " << user.clients[i] << " ";
			}
		}
	}
	cout << endl;
}