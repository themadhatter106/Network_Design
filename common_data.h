/*
	common_data.h:  header file containing all common definitions and data types

	Author: Chris Leger/Achintya Goel
*/

#include <stdlib.h>
#include <stdio.h>

#define PACKET_SIZE 512
#define DATALEN PACKET_SIZE-8

// data structures for data transmission packet and header information packet

struct data_packet
{
	int packet_number;
	char data[DATALEN];
	unsigned short sequence_number;
	unsigned short checksum;
};

struct header_packet
{
	int packet_number;
	int file_size;
	char mode;
	char file_name[30];
	unsigned short sequence_number;
	char intentional_corruption;
	unsigned short checksum;
};

// function prototypes
int corrupt(struct data_packet packet);
int corrupt_h(struct header_packet header);
unsigned short header_CSI(struct header_packet header);
unsigned short data_CSI(struct data_packet packet);
unsigned short checksumAndInvert(char *data, int length);
