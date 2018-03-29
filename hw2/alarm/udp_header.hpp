#ifndef UDP_HEADER_H
#define UDP_HEADER_H

#define BUFFER_SIZE 1400
#define PACKET_SIZE 1460

struct HEADER{
	bool fin;
	bool start;
	int offset;
	int data_size;
};

#endif