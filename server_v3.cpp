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
#include<sstream>
#define FD_MAXSIZE 15
#define LISTENQ 15 //maximun of listenfd queue
#define MAXLINE 1500
#define SERV_PORT 6780
using namespace std;

void broadcast(int* client, int max, int current, string message);
void YELL(int* client, int max, int current, vector<string> &client_name, vector<string> &str);
void TELL(int* client, int max, int current, vector<string> &client_name, string recv_name, vector<string> &str);
void NAME(int* client, int max, int current, vector<string> &client_name, string new_name);
void WHO(int* client, int max, int current, vector<string> &client_name, struct sockaddr_in* client_info);
//vector<string> preprocess(char* buffer);
vector<string> preprocess(string buffer);

int main(int argc, char** argv){
	int maxfd, listenfd, connfd, sockfd;
	int max, i;
	int ready, client[FD_MAXSIZE];
	ssize_t n;
	char buffer[MAXLINE];
	string message;
	fd_set rset, allset;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	struct sockaddr_in client_info[FD_MAXSIZE];

	vector<string> client_name(FD_MAXSIZE, "anonymous"); //store client name
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //host to network byte order long
	if(argc!=1){
		int port = atoi(argv[1]);
		servaddr.sin_port = htons(port);
	}
	else
		servaddr.sin_port = htons(SERV_PORT); //host to network byte order short
	
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	listen(listenfd, LISTENQ);

	maxfd = listenfd;
	max = -1;
	for(int i=0;i<FD_MAXSIZE;++i)
		client[i] = -1; //-1 indicates available entry
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	while(1){
		rset = allset;
		if(select(maxfd+1, &rset, NULL, NULL, NULL)==-1){
			perror("select failed\n");
			exit(1);
		}
		
		if(FD_ISSET(listenfd, &rset)){ //new client connection
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
			
			for(i=0;i<FD_MAXSIZE;++i){
				
				if(client[i]<0){
					client[i] = connfd; //save descriptor
					FD_SET(connfd, &allset);
					
					//cout << "client connect success\n";
					char send_message[MAXLINE];
					sprintf(send_message, "[Server] Hello, anonymous! From: %s/%d\n", inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
					write(client[i], send_message, strlen(send_message));
					client_info[i].sin_addr = cliaddr.sin_addr;
					client_info[i].sin_port = cliaddr.sin_port;
					
					message = "[Server] Someone is Coming!\n";
					broadcast(client, FD_MAXSIZE, i, message);//broadcast to other users
					break;
				}

			}

			if(i==FD_MAXSIZE){
				message = "[Server] ERROR: Sorry, too much client. Try again later.\n";
				write(connfd, message.c_str(), message.length());
				close(connfd);
			}

			
			if(connfd>maxfd)
				maxfd = connfd;
			if(i>=max)
				max = i+1;//max number of clients + 1
		}
		
		for(i=0;i<max;++i){ //check all clients of data
			if(client[i]<0)
				continue;
			if(FD_ISSET(client[i], &rset)){ //whether there is input from the socket
				sockfd = client[i];
				memset(buffer, 0, sizeof(buffer));//clear the buffer
				if((n=read(sockfd, buffer, MAXLINE))==0){ //connection closed by client
					string username = client_name[i];
					client_name[i] = "anonymous"; //reset the client name
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
					message = "[Server] "+ username +" is offline\n";
					broadcast(client, max, i, message);
					//cout << username << " is offline\n";
				}
				
				else{//server response to  client's request
					if(strcmp(buffer,"\n")==0 || strcmp(buffer, "\r\n")==0)
						continue;
					vector<string> request = preprocess(buffer);
					
					if(request[0]=="yell" && request.size()>=2)
						YELL(client, max, i, client_name, request);
					else if(request[0]=="tell" && request.size()>=3)
						TELL(client, max, i, client_name, request[1], request);
					else if(request[0]=="name" && request.size()==2)
						NAME(client, max, i, client_name, request[1]);
					else if(request[0]=="who")
						WHO(client, max, i, client_name, client_info);
					else{ //error command
						message = "[Server] ERROR: Error command.\n";
						write(client[i], message.c_str(), message.length());
					}
				}
			}
		}
	}
	
	return 0;
}

void broadcast(int* client, int max, int current, string message){
	for(int i=0;i<max;++i){
		if(client[i]<0 || i==current)
			continue;
		else
			write(client[i], message.c_str(), message.length());
	}

	return;
}

void YELL(int* client, int max, int current, vector<string> &client_name, vector<string> &str){
	string message;
	//message = "[Server] "+client_name[current]+" yell "+str+"\n";
	message = "[Server] "+client_name[current]+" yell";
	for(int i=1;i<str.size();++i){
		message += " ";
		message += str[i];
	}
	message+="\n";
	broadcast(client, max, -1, message);
	return;
}

void TELL(int* client, int max, int current, vector<string> &client_name, string recv_name, vector<string> &str){
	int sockfd = client[current];
	string message;
	message = "[Server] "+client_name[current]+" tell you";
	for(int i=2;i<str.size();++i){
		message += " ";
		message += str[i];
	}
	message+="\n";
	
	if(recv_name=="anonymous"){
		message = "[Server] ERROR: The client to which you sent is anonymous.\n";
		write(sockfd, message.c_str(), message.length());
		return;
	}
	if(client_name[current]=="anonymous"){ //sender cannot be anonymous
		message = "[Server] ERROR: You are anonymous.\n";
		write(sockfd, message.c_str(), message.length());
		return;
	}
	int k;
	for(k=0;k<max;++k){
		if(client_name[k]==recv_name){
			//message = "[Server] "+client_name[current]+" tell you "+str+"\n";
			write(client[k], message.c_str(), message.length());
			break;
		}
	}
	if(k==max){
		message = "[Server] ERROR: The receiver doesn't exist.\n";
		write(sockfd, message.c_str(), message.length());
	}
	else{
		message = "[Server] SUCCESS: Your message has been sent.\n";
		write(sockfd, message.c_str(), message.length()); //to sender
	}
	
	return;
}

void NAME(int* client, int max, int current, vector<string> &client_name, string new_name){
	string message;
	int sockfd = client[current];
	
	if(new_name=="anonymous"){
		message = "[Server] ERROR: Username cannot be anonymous.\n";
		write(sockfd, message.c_str(), message.length());
		return;
	}
	else if(new_name.length()<2 || new_name.length()>12){
		message = "[Server] ERROR: Username can only consists of 2~12 English letters.\n";
		write(sockfd, message.c_str(), message.length());
		return;
	}
	else{
		//check if all character is english letter
		for(int j=0;j<new_name.length();++j){
			int tmp = new_name[j];
			if((tmp>='A'&&tmp<='Z')||(tmp>='a'&&tmp<='z'))
				continue;
			else{
				message = "[Server] ERROR: Username can only consists of 2~12 English letters.\n";
				write(sockfd, message.c_str(), message.length());
				return;
			}
		}

		//find same username
		for(int i=0;i<max;++i){
			if(new_name==client_name[i] && i!=current){
				message = "[Server] ERROR: "+ new_name +" has been used by others.\n";
				write(sockfd, message.c_str(), message.length());
				return;
			}
		}
		//client[i]'s new name set
		string old_name = client_name[current]; 
		client_name[current] = new_name;
		message = "You're now known as "+new_name+".\n";
		write(sockfd, message.c_str(), message.length());

		message = "[Server] "+ old_name +" is now known as "+new_name+".\n";
		broadcast(client, max, current, message);

	}
	
	return;

}

void WHO(int* client, int max, int current, vector<string> &client_name, struct sockaddr_in* client_info){
	char send_message[MAXLINE];
	int sockfd = client[current];
	for(int k=0;k<max;++k){
		if(client[k]<0)
			continue;
		else if(k==current){
			sprintf(send_message, "[Server] %s %s/%d ->me\n", client_name[k].c_str() ,inet_ntoa(client_info[k].sin_addr),ntohs(client_info[k].sin_port)); 
			write(sockfd, send_message, strlen(send_message));
		}
		else{
			sprintf(send_message, "[Server] %s %s/%d\n", client_name[k].c_str() ,inet_ntoa(client_info[k].sin_addr),ntohs(client_info[k].sin_port)); 
			write(sockfd, send_message, strlen(send_message));
		}
	}
	return;
}

/*vector<string> preprocess(char* buffer){
	int len = strlen(buffer);
    if(buffer[len-1]=='\n')
        buffer[len-1] = '\0';
	if(buffer[len-2]=='\r')
		buffer[len-2] = '\0';
	
	vector<string> request;
	char* tmp = strtok(buffer, " ");
	string str=tmp;
	request.push_back(str);
	while(tmp!=NULL){
		tmp = strtok(NULL, " ");
		if(tmp==NULL)
			break;
		str = tmp;
		request.push_back(str);
	}
	return request;
}*/

vector<string> preprocess(string buffer){
	vector<string> request;
    string token;
    stringstream ss(buffer);
	//ss << buffer;
    
    while(ss >> token)request.push_back(token);
	
	return request;
}
