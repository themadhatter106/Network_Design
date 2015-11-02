/*
   UDP file client
   PHASE 4

   Programmer: Chris Leger

   Reference: http://www.binarytides.com/udp-socket-programming-in-winsock/
*/
#include<stdio.h>
#include<winsock2.h>
#include<string.h>
#include<stdlib.h>
#include<windows.h>
#include<time.h>
#include"common_data.h"

 
#pragma comment(lib,"ws2_32.lib") //Winsock Library
 
#define SERVER "127.0.0.1"  //ip address of udp server
#define PORT 8888   //The port on which to listen for incoming data
#define CORRUPTION_PROB 40 //DATA and ACK Corruption percentage


int main(void)
{
    struct sockaddr_in si_other;
    int s, slen=sizeof(si_other);
	char file_name[30];
	char server[12];
	int read_ret;
	char prog_mode;
	char intentional_corruption;
	int dropped_prob;
	int file_length_left;
	int file_length;
	int bytes_written = 0;
	int packet_count;
	int r,c,i,j;
	int random;
	char **data_storage;
	int sequence;
	int prev_sequence;
	FILE* fp;
    WSADATA wsa;
	int recv_status;

	//packet sent preceding the data containing information to initiate the file transfer
	
	

	struct data_packet send_data;
	struct data_packet receive_data;
	struct data_packet ACK0;
	struct data_packet ACK1;
	struct header_packet header;

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


	//Initialise winsock
    printf("\nInitialising Winsock...");
    if (WSAStartup(MAKEWORD(2,2),&wsa) != 0)
    {
        printf("Failed. Error Code : %d",WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Initialised.\n");
     
    //create socket
    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
    {
        printf("socket() failed with error code : %d" , WSAGetLastError());
        exit(EXIT_FAILURE);
    }
     
	printf("For localhost enter: 127.0.0.1 \n");
	printf("Enter Server IP Address: ");
		scanf("%s", &server);
	


    //setup address structure
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
    si_other.sin_addr.S_un.S_addr = inet_addr(server);

	//seed the random number generator
	srand((int)time(NULL));

    //start communication
    while(1)
    {
		bytes_written = 0;
		packet_count = 0;
		sequence = 0;
		prev_sequence = 1;


		printf("Do you want to send a file? <y/n>: ");
		scanf(" %c", &prog_mode);
		printf("Do you want intentional data corruption? <y/n>: ");
		scanf(" %c", &intentional_corruption);
		
		if (intentional_corruption == 'y'){

			printf("Options: \nACK packet bit-error = A\nData packet bit-error = B \n");
			printf("Please enter your selection <A/B>: ");
			scanf(" %c", &intentional_corruption);

		}

		printf("Enter percentage of dropped packets <0-100>: ");
		scanf( " %i", &dropped_prob);

		//client will receive a file
			if(prog_mode == 'n'){

				if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_0, sizeof(timeout_0)) == SOCKET_ERROR)
				{
					printf("setsockopt() failed with error code : %d" , WSAGetLastError());
				}

				printf("Enter file name to read on server : ");
				scanf("%s",header.file_name);
				printf("Enter file name to write on client : ");
				scanf("%s",file_name);
		

				fp = fopen(file_name,"wb");


				//make first packet (header_packet) requesting file to be sent

		

				header.file_size = 0;
				header.mode = 'n';
				header.packet_number = packet_count;
				header.intentional_corruption = intentional_corruption;
				header.checksum = 0;
				header.checksum = header_CSI(header);


				printf("\n HEADER INFORMATION \n");
				printf("packet number = %i \n",header.packet_number);
				//mode dictates whether the server should send or receive
				printf("mode = %c \n",header.mode);
				printf("File name = %s \n\n",header.file_name);

				do {
					//send header packet asking to send file

					send_packet((char*)&header,sizeof(struct header_packet),s,&si_other,slen);

					//wait for ACK for header
					printf("waiting for ACK from server \n");

					//in case of corrupt header packet or corrupt ACK, repeat
				} while(receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen) == -1 ||
					corrupt(receive_data) || isACK(receive_data,prev_sequence));
						
				//notification for successful recepit of ACK
				if(isACK(receive_data,sequence) == 1){

					printf("received ACK for header packet from server \n");

				}else{

					printf("ERROR: UNKNOWN DATA RECEIVED \n");
				}


				//wait for return header packet from server indicating the file size

				printf("Waiting for header packet from server...\n");
	
				//get header packet from socket
				receive_packet((char*)&header,sizeof(struct header_packet),s,&si_other,slen);

				
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
				printf("mode = %c \n",header.mode);
				printf("File size = %i \n",header.file_size);




				//Allocate array to hold data for packet reordering

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










				printf("Waiting for data...\n");
				fflush(stdout);
         
        
         
       
				//try to receive some data, stop when you recieve all the data which the file contains

				while(bytes_written < header.file_size){

					//clear out the buffer each time to eliminate extraneous data on the last packet
					memset(receive_data.data,0xFF, DATALEN);

					
					//randomly pull the waiting packet from the socket to simulate a lost data packet
					if(dropped_prob > 0){

						
						random = rand() % 100;
						if(random > dropped_prob)
							printf("DROPPED DATA PACKET!\n");
						if(random <= dropped_prob){
							receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);
						}

					}



		
					//listen for packet and store into data_packet struct

					receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);


					
					//intentionally corrupt data packet if user selected
					if(intentional_corruption == 'B'){

						
						random = rand() % 100;
						//randomly choose data packets and invalidate the sequence number

						if(random < CORRUPTION_PROB){
							receive_data.sequence_number = 49;
						}

					}

					//if data is corrupt or has the wrong sequence number

					while(corrupt(receive_data)||receive_data.sequence_number == prev_sequence){

						
						printf("CORRUPT OR WRONG SEQUENCE DATA PACKET RECEIVED \n");
						printf("RESENDING PREVIOUS ACK \n\n");

						//resend the prev_sequence ACK packet 

						
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

						printf("received data%i from server \n", sequence);

					}else{


						printf("ERROR: UNKNOWN DATA RECEIVED \n");
					}
					

					//randomly choose to send or not send the ACK to the server
					if(dropped_prob > 0){

						
						random = rand() % 100;

						if(random< dropped_prob)
							printf("DROPPED ACK PACKET!\n");
						if(random >= dropped_prob){
							//send the ACK packet to the server
							if(sequence == 0){
								printf("Sending ACK0 to server \n");
								send_packet((char*)&ACK0,sizeof(struct data_packet),s,&si_other,slen);
						
							}else{
								printf("Sending ACK1 to server \n");
								send_packet((char*)&ACK1,sizeof(struct data_packet),s,&si_other,slen);

							}
						}

					}else{
						//send the ACK packet to the server
						if(sequence == 0){
								printf("Sending ACK0 to server \n");
								send_packet((char*)&ACK0,sizeof(struct data_packet),s,&si_other,slen);
						
						}else{
								printf("Sending ACK1 to server \n");
								send_packet((char*)&ACK1,sizeof(struct data_packet),s,&si_other,slen);
						}
					}
					



					//copy data received into array to reorder packets

					memcpy(data_storage[receive_data.packet_number-1], receive_data.data, (DATALEN));

					printf("packet number = %i \n", receive_data.packet_number);
         
					//print the data received data and origin
		 
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

				//copy data into file in the correct order until the file is the correct size

				while(bytes_written != header.file_size){

				//copy array

					

					for(i=0; i<r; i++){
						bytes_written = bytes_written + DATALEN;
						printf("Writing Packet#: %i \n", i+1);
						
						if(bytes_written < header.file_size){
							fwrite(data_storage[i],1,DATALEN,fp);
							printf("bytes_written: %i \n",bytes_written);
						
						//special case for the last packet which isen't DATALEN bytes long
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
		
		



			//client will send a file
			if(prog_mode == 'y'){

				//set socket timeout

				if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == SOCKET_ERROR)
				{
					printf("setsockopt() failed with error code : %d" , WSAGetLastError());
				}

				// get file name to be sent from user



				printf("Enter file name to read on client : ");
				scanf("%s",file_name);
				printf("Enter file name to write on server : ");
				scanf("%s",header.file_name);


				fp = fopen(file_name,"rb");



				if (fp == NULL){
					printf("Invalid file name \n");
					exit(EXIT_FAILURE);
				}

				//find length of the file
				fseek(fp, 0L, SEEK_END);
				file_length = ftell(fp);
				file_length_left = file_length;
				rewind(fp);

		
				//make first packet (header_packet) containing file length

		

				header.file_size = file_length;
				header.mode = 'y';
				header.packet_number = packet_count;
				header.intentional_corruption = intentional_corruption;
				header.checksum = header_CSI(header);

				printf("\n HEADER INFORMATION \n");
				printf("packet number = %i \n",header.packet_number);
				//mode dictates whether the server should send or receive
				printf("mode = %c \n",header.mode);
				printf("File size = %i \n\n",header.file_size);

		
				//send header packet

				send_packet((char*)&header,sizeof(struct header_packet),s,&si_other,slen);

				//wait for ACK for header
				printf("waiting for ACK from server \n");

				recv_status = receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);
					
				
				//in case of corrupt header packet or corrupt ACK 
					while(recv_status == -1 || corrupt(receive_data)||isACK(receive_data,prev_sequence)){
						
						//resend the header packet 
						send_packet((char*)&header,sizeof(struct header_packet),s,&si_other,slen);
						
						//look for new ACK from the server
						
						recv_status = receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);
						
					}

					//notification for successful recepit of ACK
					if(isACK(receive_data,sequence) == 1){

						printf("received ACK for header packet from server \n");

					}else{

						printf("ERROR: UNKNOWN DATA RECEIVED \n");
					}








				//break up message into chunks for UDP packets

				while(file_length_left > 0){
		
					
				
					packet_count++;



					memset(send_data.data,'\0', DATALEN);
					send_data.packet_number = packet_count;

					read_ret=fread(send_data.data, 1, (DATALEN), fp);
			
					if(read_ret != DATALEN && file_length_left>DATALEN){
						printf("\n ERROR, did not read DATALEN \n");
						printf("read_ret = %i \n", read_ret);
					}
			
			

					//insert sequence number
					send_data.sequence_number = sequence;

					//compute and store checksum into packet
					send_data.checksum = data_CSI(send_data);

					file_length_left = file_length_left - (DATALEN);
			
					printf("packet number = %i \n",send_data.packet_number);
			
					if(file_length_left > 0){
						printf("file length left: %i \n", file_length_left);
					}else{
						printf("file length left: 0 \n");
					}
			

				
					
					//randomly choose to not send certain data packets to the server to simulate a dropped packet
					if(dropped_prob > 0){

						
						random = rand() % 100;
						if(random < dropped_prob) 
							printf("DROPPED DATA PACKET! \n");
						if(random >= dropped_prob){
							//send the data_packet struct to the server
							send_packet((char*)&send_data,sizeof(struct data_packet),s,&si_other,slen);
						}
					}else{
						//send the data_packet struct to the server
						send_packet((char*)&send_data,sizeof(struct data_packet),s,&si_other,slen);
					}
					

	
				//randomly pull the ACK packet off of the socket beforehand to simulate a dropped ACK
					if(dropped_prob > 0){

						
						random = rand() % 100;

						if(random < dropped_prob){
							printf("DROPPED ACK PACKET!\n");
							receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);
						}
					}

					
					//look for ACK from the server

					printf("waiting for ACK%i from server \n", sequence);

					recv_status = receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);

					
					//intentionally corrupt the received ACK packet if user selected
					if(intentional_corruption == 'A'){

						random = rand() % 100;
						//randomly choose ACK packets and invalidate the sequence number

						if(random <= CORRUPTION_PROB){
						receive_data.sequence_number = 69;
						}

					}
					
					printf("recv_status = %i \n", recv_status);
					//if the ACK packet is corrupted or wrong sequence ACK is received
					while(recv_status == -1 || corrupt(receive_data)||isACK(receive_data,prev_sequence)){

						
						


						if(recv_status == -1){
							printf("TIMEOUT DETECTED: RESENDING PACKET\n");
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

						printf("received ACK%i from server \n", sequence);

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

			fclose(fp);

			}


		



	}
 
    closesocket(s);
    WSACleanup();
 
    return 0;
}