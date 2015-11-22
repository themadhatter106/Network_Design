/*
	common_data.h:  header file containing all common definitions and data types

	Author: Chris Leger/Achintya Goel
*/

#include <stdlib.h>
#include <stdio.h>
#include<winsock2.h>

#define PACKET_SIZE 512			//Max length of buffer
#define DATALEN PACKET_SIZE-8	//DATA length to fit structure data_packet in PACKET_SIZE bytes
static int timeout_1 = 1;
static int timeout_0 = 0;

// data structures for data transmission packet and header information packet

struct data_packet
{
	int packet_number;
	char data[DATALEN];
	unsigned short checksum;
};

struct header_packet
{
	int packet_number;
	int file_size;
	char mode;
	char file_name[30];
	char intentional_corruption;
	int corruption;
	float timer;
	int window;
	unsigned short checksum;
};

// function prototypes
int corrupt(struct data_packet packet);
int corrupt_h(struct header_packet header);
unsigned short header_CSI(struct header_packet header);
unsigned short data_CSI(struct data_packet packet);
unsigned short checksumAndInvert(char *data, int length);
int isACK(struct data_packet ACK ,unsigned short sequence_number);
int receive_packet(char* data,int size,SOCKET s,struct sockaddr_in* si_other, int slen);
int send_packet(char* data,int size,SOCKET s,struct sockaddr_in* si_other, int slen);