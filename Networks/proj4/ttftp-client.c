/*
** name: ttftp-client.c
**
** author: pikachu
** created: 31 jan 2015 by bjr
** last modified:
**		14 feb 2016, for 162 semester of csc424 -bjr 
**
** from template created 31 jan 2015 by bjr
**
*/

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<assert.h>
#include<unistd.h>

#include "ttftp.h"

#define h_addr h_addr_list[0]

int ttftp_client( char * to_host, int to_port, char * file ) {
	int block_count ;
	int sockfd ;
	int reqlen ;
	int bytes_sent ;
	struct sockaddr_in their_addr ;
	struct hostent *he ;
	struct TftpReq *req;

	reqlen = strlen(file) + strlen(OCTET_STRING) + 4;
	req = malloc(reqlen);
	*((short*)req->opcode) = htons(TFTP_RRQ);
	strcpy(req->filename_and_mode, file);
	strcpy(req->filename_and_mode + strlen(file) + 1, OCTET_STRING);

	/*
	 * create a socket to send
	 */

	if ((sockfd=socket(AF_INET,SOCK_DGRAM,0))==-1) {
		perror("socket") ;
		exit(1) ;
	}

	// get ip address for host
	if ((he=gethostbyname(to_host))==NULL) {
		perror("gethostbyname") ;
		exit(1) ;
	}

	their_addr.sin_family = AF_INET ;
	their_addr.sin_port = htons((short)to_port) ;
	their_addr.sin_addr = *((struct in_addr *)he->h_addr) ;
	memset(&(their_addr.sin_zero), '\0', 8 ) ;
	 
	/*
	 * send RRQ
	 */

	 if ((bytes_sent=sendto(sockfd, req, reqlen, 0,
			(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
		perror("sendto") ;
		exit(1) ;
	}
	free(req);

	block_count = 1 ; /* value expected */
	while ( block_count ) {
		int numbytes;
		unsigned int addr_len ;
		char buf[MAXMSGLEN] ;
		struct TftpData* data;
		struct TftpAck* ack;
		int i;

		/*
		 * read at DAT packet
		 */

		// listen for a packet
		addr_len = sizeof(struct sockaddr) ;
		if ((numbytes=recvfrom(sockfd, buf, MAXMSGLEN-1,0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1 ) {
			perror("recvfrom") ;
			exit(1) ;
		}
		 
		/*
		 * write bytes to stdout
		 */
		 
		 data = (struct TftpData*) buf;
		 for(i = 0; i < numbytes - 4; i++) {
		 	putchar(data->data[i]);
		 } 

		/*
		 * send an ACK
		 */

		ack = malloc(4);
		*((short*)ack->opcode) = htons(TFTP_ACK);
		*((short*)ack->block_num) = htons(block_count);

	 	if ((bytes_sent=sendto(sockfd, ack, 4, 0,
				(struct sockaddr *)&their_addr, sizeof(struct sockaddr)) ) == -1 ) {
			perror("sendto") ;
			exit(1) ;
		}
		free(ack);

		block_count ++ ;
		
		/* check if more blocks expected, else 
		 * set block_count = 0 ;
		 */
		if(numbytes < 516) {
			break;
		}
	}
	return 0 ;
}

