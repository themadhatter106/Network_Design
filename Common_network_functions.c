/*
	UDP file client/server common functionality files
	
	Programmer:  Achintya Goel
	
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"common_data.h"



// Function checks to see it the data packet is corrupted by computing
// the checksum value and comparing it to the one in the packet

int corrupt(struct data_packet packet)
{
	if (data_CSI(packet) == packet.checksum)
		return 0;
	else
		return 1;
}

// Function checks to see it the header packet is corrupted by computing
// the checksum value and comparing it to the one in the packet

int corrupt_h(struct header_packet header)
{
	if (header_CSI(header) == header.checksum)
		return 0;
	else
		return 1;
}

// Function to create checksum code in a header packet used for error verification

unsigned short header_CSI(struct header_packet header)
{
	struct header_packet temp_header = header;

	temp_header.checksum = 0;
	return (checksumAndInvert((char *) &temp_header, sizeof(temp_header)));
}

// Function to create checksum code in a data packet used for error verification

unsigned short data_CSI(struct data_packet packet)
{
	struct data_packet temp_data = packet;

	temp_data.checksum = 0;
	return (checksumAndInvert((char *) &temp_data, sizeof(temp_data)));
}

// Function to create checksum code in any packet used for error verification

unsigned short checksumAndInvert(char *data, int numBytes)
{
	unsigned short checksum = 0, temp = 0;

	// loop to load each individual character into the unsigned short and sum them

	while (numBytes > 0)
	{
		// insert two chars into unsigned int
		temp = *(data++);
		temp <<= 8;
		temp += *(data++);

		// add temporary result to checksum and decrement remaining byte count
		checksum += temp;
		numBytes -= 2;
	}

	// return inverted value for better error checking
	return (~checksum);
}