/*
	UDP file server
	
	Programmer:  Achintya Goel
	
	
*/

#include<stdio.h>
#include<string.h>
#include<stdlib.h>

unsigned short checksumAndInvert(char *data, int length);

void main()
{
	char data[] = { '\1', '\2', '\1', '\2' };
	
	printf("Checksum value = %d", checksumAndInvert(data, sizeof(data)));
}

// Function to create checksum code in data packet used for error verification

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