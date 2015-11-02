/*
   UDP file server
   PHASE 4

   Programmer: Chris Leger

   Reference: http://www.binarytides.com/udp-socket-programming-in-winsock/
*/
 
#include<stdio.h>
#include<winsock2.h>
#include<string.h>
#include<stdlib.h>
#include<io.h>
#include"common_data.h"
 
#pragma comment(lib,"ws2_32.lib") //Winsock Library
 
#define PORT 8888   //The port on which to listen for incoming data
#define CORRUPTION_PROB 40 //DATA and ACK Corruption percentage



int main()
{
    SOCKET s;
    struct sockaddr_in server, si_other;
    int slen;
    WSADATA wsa;
	int read_ret;
	int file_length_left;
	int file_length;
	int random;
	int bytes_written = 0;
	int packet_count = 0;
	int r,c,i,j;
	int sequence;
	int prev_sequence;
	char **data_storage;
	FILE* fp;
	



	struct data_packet receive_data;
	struct data_packet send_data;
	struct data_packet ACK0;
	struct data_packet ACK1;
	struct header_packet header;
	int recv_status;

	//initalize ACK0

	memset(ACK0.data,'+',DATALEN);
	ACK0.sequence_number = 0;
	ACK0.packet_number = -1;
	ACK0.checksum = data_CSI(ACK0);

	//initalize ACK1

	memset(ACK1.data,'+',DATALEN);
	ACK1.sequence_number = 1;
	ACK1.packet_number = -1;
	ACK1.checksum = data_CSI(ACK1);

    slen = sizeof(si_other) ;
     
    //Initialise winsock
    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Initialised.\n");
     
    //Create a socket
    if((s = socket(AF_INET , SOCK_DGRAM , IPPROTO_UDP)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d" , WSAGetLastError());
    }
    printf("Socket created.\n");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( PORT );
     
    //Bind
    if( bind(s ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    puts("Bind done \n");
 



    //keep listening for data
    while(1){

		if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_0, sizeof(timeout_0)) == SOCKET_ERROR)
	{
		printf("setsockopt() failed with error code : %d" , WSAGetLastError());
	}

    
       bytes_written = 0;
	   packet_count = 0;
	   sequence = 0;
	   prev_sequence = 1;
	

		//look for header packet from client to determine send/receive
		

		printf("Waiting for header packet from client...\n");
		
		//wait for header packet
		receive_packet((char*)&header,sizeof(struct header_packet),s,&si_other,slen);
		printf("\n HEADER INFORMATION \n");
		printf("packet number = %i \n",header.packet_number);
		//mode dictates whether the server should send or receive
		printf("mode = %c \n",header.mode);
		printf("File name = %s \n\n",header.file_name);

		//if header packet is corrupt send the previous sequence number ACK
		
		while(corrupt_h(header) == 1){
			//resend the prev_sequence ACK packet
				send_packet((char*)&ACK1,sizeof(struct data_packet),s,&si_other,slen);

				//get a new header packet
				receive_packet((char*)&header,sizeof(struct header_packet),s,&si_other,slen);
			}
	
		//send ack
		printf("sending header acknowledgement \n");
		send_packet((char*)&ACK0,sizeof(struct data_packet),s,&si_other,slen);


		printf("\n HEADER INFORMATION \n");
		printf("packet number = %i \n",header.packet_number);
		//mode dictates whether the server should send or receive
		printf("mode = %c \n",header.mode);
		printf("File size = %i \n",header.file_size);
				
	
	

	
		
				
				
				
		//Server will be sending a file		
				
		if(header.mode == 'n'){
		
			//set socket timeout

			if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == SOCKET_ERROR)
				{
					printf("setsockopt() failed with error code : %d" , WSAGetLastError());
				}
         
			// get file name to be sent from header packet

			fp = fopen(header.file_name,"rb");



			if (fp == NULL) {
				printf("Invalid file name \n");
				exit(EXIT_FAILURE);
			}

			//find length of the file
			fseek(fp, 0L, SEEK_END);
			file_length = ftell(fp);
			file_length_left = file_length;
			rewind(fp);

		
			//make another packet in response to client's (header_packet) to transmit necessary data for file transfer

		

			header.file_size = file_length;
			header.mode = 'n';
			header.packet_number = packet_count;
			header.checksum = header_CSI(header);

			printf("\n HEADER INFORMATION \n");
			printf("packet number = %i \n",header.packet_number);
			//mode dictates whether the server should send or receive
			printf("mode = %c \n",header.mode);
			printf("File size = %i \n\n",header.file_size);


			do {
				//send header packet
				send_packet((char*)&header,sizeof(struct header_packet),s,&si_other,slen);

				//wait for ACK for header
				printf("waiting for ACK from client \n");

				//in case of corrupt header packet or corrupt/wrong ACK 
			} while(receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen) == -1 ||
				corrupt(receive_data) || isACK(receive_data,prev_sequence));

			//notification for successful recepit of ACK
			if(isACK(receive_data,sequence) == 1){

				printf("received ACK for header packet from client \n");

			}else{

				printf("ERROR: UNKNOWN DATA RECEIVED \n");
			}



			while(file_length_left > 0){
		
				memset(send_data.data,'\0', DATALEN);

				packet_count++;
				send_data.packet_number = packet_count;

				read_ret=fread(send_data.data, 1, DATALEN, fp);
				
				if(read_ret != DATALEN && file_length_left>DATALEN){
					printf("\n ERROR, did not write DATALEN \n");
				}
			
			

				file_length_left = file_length_left - (DATALEN);
			
				printf("packet number = %i \n",send_data.packet_number);
			
				if(file_length_left > 0){
					printf("file length left: %i \n", file_length_left);
				}else{
					printf("file length left: 0 \n");
				}
			

				//insert sequence number
				send_data.sequence_number = sequence;

				//compute and store checksum into packet
				send_data.checksum = data_CSI(send_data);
				

				//send the data packet
				send_packet((char*)&send_data,sizeof(struct data_packet),s,&si_other,slen);
				
				
		 
				//look for ACK from the client

					printf("waiting for ACK%i from client \n", sequence);

					recv_status = receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);
					

					//intentionally corrupt the received ACK packet if user selected
					if(header.intentional_corruption == 'A'){

						random = rand() % 100;
						//randomly choose ACK packets and invalidate the sequence number

						
						if(random <= CORRUPTION_PROB ){
						receive_data.sequence_number = 49;
						}

					}

					
					//if the packet is corrupted or wrong sequence ACK is received
					while(recv_status == -1 || corrupt(receive_data)||isACK(receive_data,prev_sequence)){
						
						if(recv_status == -1){
							printf("TIMEOUT DETECTED: RESENDING PACKET \n \n");
						}else{
						
						printf("CORRUPT OR WRONG SEQUENCE ACK PACKET RECEIVED \n");
						printf("Resending Data packet \n \n");
						}
					
						
						//resend the data packet 
						send_packet((char*)&send_data,sizeof(struct data_packet),s,&si_other,slen);
						

						//look for new ACK from the server

						printf("waiting for ACK%i from server \n", sequence);
						
						recv_status = receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);

					}


					//If the correct ACK was sucessfully received
					if(isACK(receive_data,sequence) == 1){

						printf("received ACK%i from client \n", sequence);

					}else{


						printf("ERROR: UNKNOWN DATA RECEIVED \n");
					}




						prev_sequence=sequence;
						sequence++;

					//when sequence number 1 is done go back to sequence number 0
					if(sequence > 1){

						sequence = 0;
					}

				}
		
		
	
		}
		
		




		//server will receive a file
		
		if(header.mode == 'y'){

			//set socket timeout to zero

			if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_0, sizeof(timeout_0)) == SOCKET_ERROR)
	{
		printf("setsockopt() failed with error code : %d" , WSAGetLastError());
	}
		

			//Allocate array to hold data for packet reordering

			//calculate rows and columns of array
			r = ((header.file_size-(header.file_size%(DATALEN)))/(DATALEN));
			
			if((header.file_size%DATALEN)!=0){
				r++;
			}
				
			c = DATALEN;

			data_storage = (char **)malloc(r * sizeof(char*));
			
			for (i=0; i<r; i++){
				data_storage[i] = (char *)malloc(c * sizeof(char));
			}
	
			//initalize array
			for (i = 0; i <  r; i++)
				for (j = 0; j < c; j++)
					data_storage[i][j] = '+';
		
		
			fp = fopen(header.file_name,"wb");


			printf("Waiting for data...\n");
			fflush(stdout);
         
        
         
       
			//try to receive some data, stop when you recieve all the data which the file contains

			while(bytes_written < header.file_size){

				//clear out the buffer each time to eliminate extraneous data on the last packet
				memset(receive_data.data,0xFF, DATALEN);

				//look for data from client
				receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);


				//intentionally corrupt data packet if user selected
				if(header.intentional_corruption == 'B'){

					random = rand() % 100;
						//randomly choose data packets and invalidate the sequence number

						if(random <= CORRUPTION_PROB){
						receive_data.sequence_number = 49;
						}


				}

				//if data is corrupt or has the previous sequence number

				while(corrupt(receive_data)||receive_data.sequence_number == prev_sequence){
						//resend the prev_sequence ACK packet 
						printf("CORRUPT OR WRONG SEQUENCE DATA PACKET RECEIVED \n");
						printf("RESENDING PREVIOUS ACK \n\n");

					if(prev_sequence == 0){
						send_packet((char*)&ACK0,sizeof(struct data_packet),s,&si_other,slen);

					}else{
						send_packet((char*)&ACK1,sizeof(struct data_packet),s,&si_other,slen);

					}


					//look for new data from client
					receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);
					
				}


				//If the correct sequence data packet was sucessfully received
					if(receive_data.sequence_number == sequence){

						printf("received data%i from client \n", sequence);

					}else{


						printf("ERROR: UNKNOWN DATA RECEIVED \n");
					}


					


				//send the ACK packet to the client
				if(sequence == 0){
						printf("Sending ACK0 to client \n");
						send_packet((char*)&ACK0,sizeof(struct data_packet),s,&si_other,slen);
						
					}else{
						printf("Sending ACK1 to client \n");
						send_packet((char*)&ACK1,sizeof(struct data_packet),s,&si_other,slen);

					}




				//copy data received into array to reorder packets

				memcpy(data_storage[receive_data.packet_number-1], receive_data.data, (DATALEN));

				printf("packet_number = %i \n", receive_data.packet_number);
         
				//print the data received and origin
		 
				printf("\n\n\nReceived packet from %s:%d \n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
			    printf("Data: %s\n\n\n" , receive_data.data);
				bytes_written = bytes_written + DATALEN;




				prev_sequence=sequence;
						sequence++;

					//when sequence number 1 is done go back to sequence number 0
				if(sequence > 1){

					sequence = 0;
				}

				


			}
		

			bytes_written = 0;

			while(bytes_written != header.file_size){

				//copy array into file

					

				for(i=0; i<r; i++){
					bytes_written = bytes_written + DATALEN;
					printf("Writing Packet#: %i \n", i+1);
					if(bytes_written < header.file_size){
						fwrite(data_storage[i],1,DATALEN,fp);
						printf("bytes_written: %i \n",bytes_written);
					}else{
						bytes_written = bytes_written - (DATALEN);
						printf("bytes_written: %i \n",bytes_written);
							
						fwrite(data_storage[i],1,(header.file_size - bytes_written),fp);
							
						printf("PARTIAL PACKET WRITE \n");
						printf("Wrote: %i \n",(header.file_size - bytes_written));
						bytes_written += (header.file_size - bytes_written);
					}
				}
			}


			printf("Receive Done. \n");

		
			printf("header File size = %i \n",header.file_size);

			printf("End of Transmission \n");

			fclose(fp);

			//free data storage array
			for (i=0; i<r; i++){
				free(data_storage[i]);
			}
			free(data_storage);


		}

		if(header.mode != 'y' && header.mode != 'n'){

			printf("HEADER ERROR, invalid mode! \n");
			return 0;
		}

       
   }
 
    closesocket(s);
    WSACleanup();
     
    return 0;
}