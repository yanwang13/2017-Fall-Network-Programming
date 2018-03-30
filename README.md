# 2017-Fall-Network-Programming

## HW0 string processing
- reverse the string that you type
- split the string with specific character
- terminate itself by the command
- usage: ./nphw1.c [input file path] [split token]

## HW1 Multi-client Chatroom
- server
  - usage: ./server [port]
  - Hello message: [server]> Hello, anonymous! From: [client IP]/[client port]
  - Offline message: [server]> [username] is offline.
  - Who message: [server]> [username] [client IP]/[client port]
  - Change username message: [server]> You're now known as [new username]
  - Private message: [server]> [sender username] tell you [message]
  - Broadcast message: [server]> [sender username] yell [message]
  - Error command: [server]> ERROR: Error command.
  
- client
  - usage: ./client [server IP] [server port]
  - Client keeps receiving messages from stdin and sends messages to the server directly. 
  - While receiving a message from the server, the client should send to the server without modification.
  
  
## HW2 UDP Reliable File Transfer
- Receiver keeps receiving the packets from sender and output to another file.
- detect the packet loss event and re-ordering event and deal with it 
- hand-in 3 kinds of senders and 3 kinds of receivers using 3 different timeout methods respectively
- methods: alarm, select, setsockopt
- usage
  - sender: ./sender [receiver IP] [receiver port] [file name]
  - receiver: ./receiver [port]
- The file name of the received file should be as same as the original one.
- The receiver should keep receiving files until a Ctrl-C is entered.

## HW3 Non-Blocking File Transfer
- a single process & single thread non-blocking server
- The concept of this homework is like dropbox
- user can save his files on the server and the clients of the user are running on different hosts.
- when any of client of the user upload a file, the server have to transmit it to all the other clients of the user
- when a new client of the user connects to the server, the server should transmit all the files, which are uploaded by the other clients of the user, to the new client immediately
- different clients may upload different files at the same time
- if one of the client is sleeping, server has to send the data to the other clients of the user in a non-blocking way.
- usage
  - server: ./server [port]
  - client: ./client [server ip] [port] [username]
- commands:
  - /put [filename]
  - /sleep [seconds]
  - /exit
