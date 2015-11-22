/*
   UDP file client
   PHASE 5

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
	int sequence;
	int GBN_lower;
	int GBN_upper;
	int *timers;
	float timeout;
	int start_time;
	int timeout_indicator;
	int no_ACK_indicator;
	int dropped_count;
	int random;
	int previous_ack;
	int corrupt_status;
	char **data_storage;
	int CORRUPTION_PROB = 0;
	FILE* fp;
    WSADATA wsa;
	int recv_status;
	int window_size;
	struct data_packet* send_packets;

	//packet sent preceding the data containing information to initiate the file transfer
	
	
	struct data_packet temp;
	struct data_packet send_data;
	struct data_packet receive_data;
	struct data_packet ACK;
	struct header_packet header;

	//initalize ACK

	memset(ACK.data,'+',DATALEN);
	ACK.packet_number = 0;
	ACK.checksum = data_CSI(ACK);

	


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
		timeout_indicator = 0;


		printf("Do you want to send a file? <y/n>: ");
		scanf(" %c", &prog_mode);
		printf("Do you want intentional data corruption? <y/n>: ");
		scanf(" %c", &intentional_corruption);
		
		if (intentional_corruption == 'y'){

			printf("Options: \nACK packet bit-error = A\nData packet bit-error = B \n");
			printf("Please enter your selection <A/B>: ");
			scanf(" %c", &intentional_corruption);
			printf("Enter Corruption Probability <0-100>: ");
			scanf(" %i", &CORRUPTION_PROB);

		}

		printf("Enter percentage of dropped packets <0-100>: ");
		scanf( " %i", &dropped_prob);

		printf("Enter GBN window size: ");
		scanf(" %i", &window_size);

		printf("Enter timeout in Seconds: ");
		scanf(" %f", &timeout);


		start_time = clock();

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
				header.window = window_size;
				header.timer = timeout;
				header.corruption = CORRUPTION_PROB;
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
					corrupt(receive_data) || !isACK(receive_data,sequence));
						
				//notification for successful recepit of ACK
				if(isACK(receive_data, sequence) == 1){

					printf("received ACK for header packet from server \n");

				}else{

					printf("ERROR: UNKNOWN DATA RECEIVED \n");
				}


				//wait for return header packet from server indicating the file size

				printf("get header packet from server...\n");
	
				//get header packet from socket
				receive_packet((char*)&header,sizeof(struct header_packet),s,&si_other,slen);

				
				//if header packet is corrupt do nothing and wait for a new packet
		
				while(corrupt_h(header) == 1){

					//get a new header packet
					receive_packet((char*)&header,sizeof(struct header_packet),s,&si_other,slen);
				}
	

				//send ack
				printf("sending header acknowledgement \n");
				ACK.packet_number = sequence;
				ACK.checksum = data_CSI(ACK);
				send_packet((char*)&ACK,sizeof(struct data_packet),s,&si_other,slen);




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

					sequence++;

					//clear out the buffer each time to eliminate extraneous data on the last packet
					memset(receive_data.data,0xFF, DATALEN);

					
					//randomly pull the waiting packet from the socket to simulate a lost data packet
					if(dropped_prob > 0){

						
						random = rand() % 100;
						
						if(random <= dropped_prob){
							printf("DROPPED DATA PACKET!\n");
							temp = receive_data;
							receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);
							receive_data = temp;

						}

					}



		
					//listen for packet and store into data_packet struct

					receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);


					
					//intentionally corrupt data packet if user selected
					if(intentional_corruption == 'B'){

						
						random = rand() % 100;
						//randomly choose data packets and invalidate the sequence number

						if(random < CORRUPTION_PROB){
							receive_data.packet_number = 0;
						}

					}

					//if data is corrupt or has some sequence number other than expected resend previous ACK

					while(corrupt(receive_data)||receive_data.packet_number != sequence){

						
						printf("CORRUPT OR WRONG SEQUENCE DATA PACKET RECEIVED, packet = %i, sequence = %i \n",receive_data.packet_number,sequence);
						printf("RESENDING ACK FOR LAST PACKET \n\n");


						ACK.packet_number = sequence-1;
						ACK.checksum = data_CSI(ACK);
						printf("Sending ACK %i to client \n", sequence-1);
						send_packet((char*)&ACK,sizeof(struct data_packet),s,&si_other,slen);


						


						//look for new data from client
						receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);

					}


					//If the correct sequence data packet was sucessfully received
					if(receive_data.packet_number == sequence){

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
							
								printf("Sending ACK %i to server \n", sequence);
								ACK.packet_number = sequence;
								ACK.checksum = data_CSI(ACK);
								send_packet((char*)&ACK,sizeof(struct data_packet),s,&si_other,slen);

						}

					}else{
						//send the ACK packet to the server
						printf("Sending ACK %i to server \n", sequence);
						ACK.packet_number = sequence;
						ACK.checksum = data_CSI(ACK);
						send_packet((char*)&ACK,sizeof(struct data_packet),s,&si_other,slen);
					}
					



					//copy data received into array to reorder packets

					memcpy(data_storage[receive_data.packet_number-1], receive_data.data, (DATALEN));

					printf("packet number = %i \n", receive_data.packet_number);
         
					//print the data received data and origin
		 
					printf("\n\n\nReceived packet from %s:%d \n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
					printf("Data: %s\n\n\n" , receive_data.data);
					bytes_written = bytes_written + DATALEN;



					

					
	




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

				if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_1, sizeof(timeout_1)) == SOCKET_ERROR)
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
				header.corruption = CORRUPTION_PROB;
				header.checksum = header_CSI(header);

				printf("\n HEADER INFORMATION \n");
				printf("packet number = %i \n",header.packet_number);
				//mode dictates whether the server should send or receive
				printf("mode = %c \n",header.mode);
				printf("File size = %i \n\n",header.file_size);

		
				//send header packet

				send_packet((char*)&header,sizeof(struct header_packet),s,&si_other,slen);

				//wait for ACK for header
				printf("looking for ACK from server \n");

				recv_status = receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);
					
				
				//in case of corrupt header packet or corrupt ACK 
					while(recv_status == -1 || corrupt(receive_data)||!isACK(receive_data,packet_count)){
						
						//resend the header packet 
						send_packet((char*)&header,sizeof(struct header_packet),s,&si_other,slen);
						
						//look for new ACK from the server
						
						recv_status = receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);
						
					}

					//notification for successful recepit of ACK
					if(isACK(receive_data,packet_count) == 1){

						printf("received ACK for header packet from server \n");

					}else{

						printf("ERROR: UNKNOWN DATA RECEIVED \n");
					}



					//calculate number of spaces in array needed to hold all the data packets

					r = ((header.file_size-(header.file_size%(DATALEN)))/(DATALEN));
			
					if((header.file_size%DATALEN)!=0){
					r++;
					}


					//make array to hold all the data packets

				send_packets = (struct data_packet*)malloc(r * sizeof(struct data_packet));
				timers = (int*)malloc(r * sizeof(int));

				GBN_lower = 0;
				
		


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
			
			


					//compute and store checksum into packet
					send_data.checksum = data_CSI(send_data);

					file_length_left = file_length_left - (DATALEN);
			
					printf("packet number = %i \n",send_data.packet_number);
			
					if(file_length_left > 0){
						printf("file length left: %i \n", file_length_left);
					}else{
						printf("file length left: 0 \n");
					}
			

					send_packets[GBN_lower] = send_data;
				
					GBN_lower++;

				}

				packet_count = 1;


				//keep sending data until the last ACK is received from the server
				while( receive_data.packet_number != r ){

		

					if(packet_count == 1){
						
						GBN_lower = 1;
						GBN_upper = window_size;
						printf("Initalized...GBN_lower = %i and GBN_upper = %i \n", GBN_lower, GBN_upper);
					}



					while(packet_count <= GBN_upper){

						send_data = send_packets[packet_count - 1];

						printf("Sending Packet %i \n", packet_count);
					
					
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

						timers[packet_count - 1] = (int)clock();

						
						
						packet_count++;

						
					}

					
	


					

					
					//check to see if any of the timers have expired
					for(i=GBN_lower; i<=GBN_upper; i++){
						if( (clock() - timers[i-1]) > timeout*CLOCKS_PER_SEC ){
							printf("TIMEOUT DETECTED on packet %i, time = %f \n", i,(float)(clock() - timers[i-1])/(float)CLOCKS_PER_SEC);
							timeout_indicator = 1;
						}
					}
					
					//if the ACK packet is corrupted throw packet away and get new one from socket
					//if the timer for a certain ACK expires resend whole window


						if(timeout_indicator == 1){
							printf("TIMEOUT DETECTED: RESENDING WINDOW\n");

							packet_count = GBN_lower;


							while(packet_count <= GBN_upper){

								send_data = send_packets[packet_count - 1];


								printf("Timeout: Sending Packet %i \n", packet_count);
					
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

								timers[packet_count - 1] = (int)clock();

								packet_count++;

								
							}



						}




					timeout_indicator = 0;


						recv_status=0;
						i=0;
						
						//pull as many ACKs off of the socket as possible and process
					while(recv_status != -1){
						
						

						
					//randomly pull the new ack and change the packet number to the previous one to simulate a dropped ACK
						if(dropped_prob > 0 && dropped_count == 0){

						
							random = rand() % 100;
							

							if(random < dropped_prob){
								printf("DROPPED ACK PACKET!\n");
								receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);
								receive_data.packet_number--;
								receive_data.checksum = data_CSI(receive_data);
								dropped_count++;
							}

						}


						//look for ACK from the server
						recv_status = receive_packet((char*)&receive_data,sizeof(struct data_packet),s,&si_other,slen);

					
					

						//intentionally corrupt the received ACK packet if user selected
						if(intentional_corruption == 'A'){

							random = rand() % 100;
							//randomly choose ACK packets and invalidate the sequence number

							if(random <= CORRUPTION_PROB && receive_data.packet_number != GBN_upper){
							receive_data.packet_number = 0;
							}

						}

						if(recv_status == -1 && i == 0 && corrupt_status < 2 && no_ACK_indicator < 1 ){
							printf("received no ACK from the server \n");
							no_ACK_indicator++;
						}
						
						if(recv_status > 0){
							printf("received ACK%i from server \n", receive_data.packet_number);
						}

						

						if(corrupt(receive_data)){
							
							
							if(corrupt_status == 0){
							printf("CORRUPT ACK PACKET RECEIVED \n");
							printf("corrupt = %i, sequence = %i \n", corrupt(receive_data), receive_data.packet_number);
							printf("Threw away, looking for new ACK packet \n");
							}
						
							

							corrupt_status++;

						}


							//ACK all packets equal to and less than the ACK# (cumulative ACK)
							//and don't slide the window longer than the bytes in the file
						if( receive_data.packet_number >= GBN_lower && GBN_upper < r){

							GBN_lower = receive_data.packet_number+1;
							GBN_upper = GBN_lower+(window_size-1);

							if(GBN_upper > r){
								GBN_upper = r;
							}
							printf(" GBN_lower = %i, GBN_upper = %i \n", GBN_lower, GBN_upper);
						
							corrupt_status = 0;
							no_ACK_indicator = 0;
							dropped_count = 0;
						}
						
						i++;

					}

					if(corrupt_status < 2 && no_ACK_indicator < 1)
						printf("\n\n\n\n");



						

				}

			fclose(fp);
			free(timers);
			free(send_packets);

			printf("The transfer took %f seconds \n", (float)(clock()-start_time)/(float)CLOCKS_PER_SEC);
			
			
			}

		



	}
 
    closesocket(s);
    WSACleanup();
 
    return 0;
}