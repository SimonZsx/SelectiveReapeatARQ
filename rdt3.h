#ifndef RDT3_H
#define RDT3_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>

#define PAYLOAD 1000		//size of data payload of the RDT layer
#define TIMEOUT 50000		//50 milliseconds
#define TWAIT 10*TIMEOUT	//Each peer keeps an eye on the receiving
#define HEADER 4  
							//end for TWAIT time units before closing
							//For retransmission of missing last ACK
#define W 5					//For Extended S&W - define pipeline window size


//----- Type defines ----------------------------------------------------------
typedef unsigned char		u8b_t;    	// a char
typedef unsigned short		u16b_t;  	// 16-bit word
typedef unsigned int		u32b_t;		// 32-bit word 

extern float LOSS_RATE, ERR_RATE;


/* this function is for simulating packet loss or corruption in an unreliable channel */
/***
Assume we have registered the target peer address with the UDP socket by the connect()
function, udt_send() uses send() function (instead of sendto() function) to send 
a UDP datagram.
***/
int udt_send(int fd, void * pkt, int pktLen, unsigned int flags) {
	double randomNum = 0.0;

	/* simulate packet loss */
	//randomly generate a number between 0 and 1
	randomNum = (double)rand() / RAND_MAX;
	if (randomNum < LOSS_RATE){
		//simulate packet loss of unreliable send
		printf("WARNING: udt_send: Packet lost in unreliable layer!!!!!!\n");
		return pktLen;
	}

	/* simulate packet corruption */
	//randomly generate a number between 0 and 1
	randomNum = (double)rand() / RAND_MAX;
	if (randomNum < ERR_RATE){
		//clone the packet
		u8b_t errmsg[pktLen];
		memcpy(errmsg, pkt, pktLen);
		//change a char of the packet
		int position = rand() % pktLen;
		if (errmsg[position] > 1) errmsg[position] -= 2;
		else errmsg[position] = 254;
		printf("WARNING: udt_send: Packet corrupted in unreliable layer!!!!!!\n");
		return send(fd, errmsg, pktLen, 0);
	} else 	// transmit original packet
		return send(fd, pkt, pktLen, 0);
}

/* this function is for calculating the 16-bit checksum of a message */
/***
Source: UNIX Network Programming, Vol 1 (by W.R. Stevens et. al)
***/
u16b_t checksum(u8b_t *msg, u16b_t bytecount)
{
	u32b_t sum = 0;
	u16b_t * addr = (u16b_t *)msg;
	u16b_t word = 0;
	
	// add 16-bit by 16-bit
	while(bytecount > 1)
	{
		sum += *addr++;
		bytecount -= 2;
	}
	
	// Add left-over byte, if any
	if (bytecount > 0) {
		*(u8b_t *)(&word) = *(u8b_t *)addr;
		sum += word;
	}
	
	// Fold 32-bit sum to 16 bits
	while (sum>>16) 
		sum = (sum & 0xFFFF) + (sum >> 16);
	
	word = ~sum;
	
	return word;
}

//----- Type defines ----------------------------------------------------------

// define your data structures and global variables in here

u8b_t nextseqnum=1;
u8b_t expectedseqnum=1;

int ss=0;

int rdt_socket();
int rdt_bind(int fd, u16b_t port);
int rdt_target(int fd, char * peer_name, u16b_t peer_port);
int rdt_send(int fd, char * msg, int length);
int rdt_recv(int fd, char * msg, int length);
int rdt_close(int fd);
u8b_t getACK(const char *pkt);
int isACKbtw(const int k, const int s, const int e);
int hasseq(const char* pkt, const u8b_t seqnum);
int count_pkt(const char* pkt);

int count_pkt(const int len){
	int i=0;
	i = (len-len%PAYLOAD)/PAYLOAD;printf("%d,%d\n",i,len);
	i = (len-PAYLOAD*i==0)?i:i+1;
	return i;
}

void make_pkt(const u8b_t seqnum, const char* data, char*t, const int num, const int len)
{
	strcpy(t,"");
	t[0]='P';
	t[1]=(char)seqnum;
	t[2]='\0';
	strcat(t,"00");
	memcpy(t+4,data+num*PAYLOAD,len);
	u16b_t sum=checksum((u8b_t *)t,len+HEADER);
	/*
	u8b_t fst= sum >> 8 & 0xFF;
	u8b_t snd= sum & 0xFF;


	if(fst==0){
		sum = snd+60000;
	}
	if(snd==0){
		sum = sum+255;
	}*/

	memcpy(t+2, &sum, 2);
}

void make_ACK(const u8b_t seqnum, char*t)
{
	strcpy(t,"");
	strcat(t,"A");
	t[1]=(char)seqnum;
	t[2]='\0';
	strcat(t,"00");
	u16b_t sum=checksum((u8b_t *)t,strlen(t));
/*
	u8b_t fst= sum >> 8 & 0xFF;
	u8b_t snd= sum & 0xFF;
	if(fst==0){
		sum = snd+60000;
	}
	if(snd==0){
		sum = sum+255;
	}*/
	memcpy(t+2, &sum, 2);
}
u8b_t getACK(char *pkt){

	u8b_t ack= (u8b_t)pkt[1];
	return ack;

}

int isACKbtw(const int k, const int s, const int e){
	int start=s;
	int real=s;
	int end=e;
	if(e==0){end=255;}
	if(e<s)
	{
		return (k==start||k==end);
	}
	for(;;)
	{
		real=start>255?start-255:start;
		if(real==k){return 1;}
		if(start==end){return 0;}
		else{start++;}
	}
}

int hasseq(const char* pkt, const u8b_t seqnum){
    int hass=((u8b_t)pkt[1]-seqnum==0);
    printf("rdt_recv: Message has seq_num:%d expected is:%d, so:%d \n",(u8b_t)pkt[1],seqnum,hass);
    return hass;
}

int corrupt(const char* pkt, const int len){

	char pktcopy[PAYLOAD+5]="";
	memcpy(pktcopy,pkt,len);
	
	pktcopy[2]='0';
	pktcopy[3]='0';

	u16b_t sum=checksum((u8b_t *)pktcopy,len);
/*
	u8b_t fst= sum >> 8 & 0xFF;
	u8b_t snd= sum & 0xFF;

	if(fst==0){
		sum = sum+60000;
	}
	if(snd==0){
		sum = sum+255;
	}
*/	
	u16b_t origin;

	memcpy(&origin,pkt+2,2);
	
	if(origin == sum){return 0;}
	else{printf("Pkt corrupt: expect CheckSum: %d, actual CheckSum: %d\n",sum,origin);return 1;}
}

int rdt_socket() {
//same as part 1
return socket(AF_INET, SOCK_DGRAM, 0);
}

/* Application process calls this function to specify the IP address
   and port number used by itself and assigns them to the RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_bind(int fd, u16b_t port){
//same as part 1
struct sockaddr_in myaddr;
    myaddr.sin_family=AF_INET;
    myaddr.sin_port=htons(port);
    myaddr.sin_addr.s_addr=INADDR_ANY;
    return bind(fd,(struct sockaddr*)&myaddr, sizeof myaddr);
}

/* Application process calls this function to specify the IP address
   and port number used by remote process and associates them to the 
   RDT socket.
   return	-> 0 on success, -1 on error
*/
int rdt_target(int fd, char * peer_name, u16b_t peer_port){
//same as part 1
    struct sockaddr_in peer_addr;
    struct hostent *he;

    if ((he=gethostbyname(peer_name)) == NULL) {  // get the host info 
        perror("gethostbyname:");
        exit(1);
    }

    peer_addr.sin_family = AF_INET;    // host byte order 
    peer_addr.sin_port = htons(peer_port);  // short, network byte order 
    peer_addr.sin_addr = *((struct in_addr *)he->h_addr); // already in network byte order
    memset(&(peer_addr.sin_zero), '\0', 8);  // zero the rest of the struct 

    return connect(fd, (struct sockaddr *)&peer_addr, sizeof(struct sockaddr));
}


/* Application process calls this function to transmit a message to
   target (rdt_target) remote process through RDT socket; this call will
   not return until the whole message has been successfully transmitted
   or when encountered errors.
   msg		-> pointer to the application's send buffer
   length	-> length of application message
   return	-> size of data sent on success, -1 on error
*/


fd_set readfds;

int rdt_send(int fd, char * msg, int length){
//implement the Extended Stop-and-Wait ARQ logic

//must use the udt_send() function to send data via the unreliable layer
msg[length]='\0';


//implement the sndpkt[]
char *pkt =msg;
char sndpkt[W*2][PAYLOAD*2];

int result;
int wbegin=nextseqnum;

struct timeval timeout;

int N=W;
u8b_t S;

//implement the sender window with acklist
//Set the Sequence number 1-255 with 1 u8b_t size

int acklist[256];
int pktLen[W];
int n;
for(n=0;n<256;n++)
{
	acklist[n]=0;
}
N = count_pkt(length);

printf("rdt_send: Going to send %d pkts with length: %d\n",N,length);

//start sender 
for(;;)
{
	if(ss==0){ // start sender 
			
		S = nextseqnum;
		int i;
		for(i=0;i<N;i++)
		{
			pktLen[i]=(length-i*PAYLOAD)>PAYLOAD?PAYLOAD:length-i*PAYLOAD;
			make_pkt(nextseqnum,pkt,sndpkt[i],i,pktLen[i]);
			udt_send(fd,sndpkt[i],pktLen[i]+HEADER,0);
			printf("rdt_send: Send one message of size:%d Seqnum:%d \n",pktLen[i],nextseqnum);
			if((int)nextseqnum==255){nextseqnum=1;}
			else {nextseqnum++;}
		}
		ss=1;
		//start timer		
		FD_ZERO(&readfds);
		FD_SET(fd,&readfds);
		continue;
	}

	else   { // goint to receive ACK
	        timeout.tv_sec = 0;
		timeout.tv_usec = TIMEOUT;
		FD_SET(fd,&readfds);
		result = select(fd+1,&readfds,NULL,NULL,&timeout);
		switch(result) 
		{
		 case 0:
			printf("rdt_send: Timeout!!!! \n");
			//retransmit all unacked sndpkt[]
			int ret;
			int retnum;
			for(ret=0;ret<N;ret++)
			{
				retnum = (wbegin+ret)>255?wbegin+ret-255:wbegin+ret;
				if(acklist[retnum]==0)
				{
					udt_send(fd,sndpkt[ret],pktLen[ret]+HEADER,0);
					printf("rdt_send: Retransmit pkt with size:%d Seqnum:%d\n",pktLen[ret]+HEADER,retnum);	
				}
			}
			timeout.tv_sec =0 ;
			timeout.tv_usec = TIMEOUT;
			continue;
		 case -1:
			perror("select");
			exit(1);
		 default:
			if(FD_ISSET(fd,&readfds)){			
				char rcvpkt[PAYLOAD]="";
				int rv=recv(fd,rcvpkt,sizeof rcvpkt,0);
				if(rv!=-1){
					u8b_t k=getACK(rcvpkt);
					if(corrupt(rcvpkt,HEADER)||!isACKbtw(k,S,S+N-1)){printf("rdt_send: Receive Corrupt/Wrong ACK:%d \n",k);continue;}
					else if(!corrupt(rcvpkt,HEADER)&&(isACKbtw(k,S+N-1,S+N-1)))
					{printf("rdt_send: Receive ACK:%d \n",k);ss=0;FD_CLR(fd,&readfds);return length;}
					else if(!corrupt(rcvpkt,HEADER)&&isACKbtw(k,S,S+N-2))
					{
						//set all sndpkt[] between S to k as acked
						printf("rdt_send: Receive ACK:%d \n",k);
						int sS=S;									
						for (;;)
						{
							acklist[sS]=1;
							if(sS==k){break;}
							sS=(sS==255)?sS-255:sS+1;
							
						}
						continue;
					}
					else {continue;}
				}
				else{printf("NO receive\n");continue;}
			}
			else{continue;}
		}
	}
}	
}

/* Application process calls this function to wait for a message of any
   length from the remote process; the caller will be blocked waiting for
   the arrival of the message. 
   msg		-> pointer to the receiving buffer
   length	-> length of receiving buffer
   return	-> size of data received on success, -1 on error
*/
int rdt_recv(int fd, char * msg, int length){
//implement the Extended Stop-and-Wait ARQ logic
char rcvpkt[PAYLOAD+5]="";
char sndpkt[PAYLOAD+5]="";
for(;;){
	strcpy(rcvpkt,"");
	int rv=recv(fd,rcvpkt,PAYLOAD+HEADER,0);
	printf("rdt_recv: Receive message with size: %d \n",rv);
	if(rv!=-1){
		if((!corrupt(rcvpkt,rv))&&rcvpkt[0]=='A'){continue;}
		else if(!corrupt(rcvpkt,rv)&&!hasseq(rcvpkt,expectedseqnum))
		{
			// Not expected pkt send ACK expectedseqnum-1
			int esn=(expectedseqnum==1)?255:expectedseqnum-1;
			make_ACK(esn,sndpkt);
			printf("rdt_recv: Receive Not Expected message and Send ACK: %d\n", esn);
			udt_send(fd,sndpkt,length+HEADER,0);
			continue;
		}
		else if((!corrupt(rcvpkt,rv))&&hasseq(rcvpkt,expectedseqnum))
		{
			//The expected package
			make_ACK(expectedseqnum,sndpkt);	
			udt_send(fd,sndpkt,PAYLOAD+HEADER,0);printf("rdt_recv: Receive Expected message and Send ACK: %d \n", expectedseqnum );
			if(expectedseqnum==255){expectedseqnum=1;}else{expectedseqnum++;}
			memcpy(msg,rcvpkt+HEADER,rv-HEADER);
			return (rv-HEADER);		
		}
		else {continue;}
	}//end if

}//end for
}//end rdt_recv

/* Application process calls this function to close the RDT socket.
*/
int rdt_close(int fd){
	//implement the Extended Stop-and-Wait ARQ logic
	fd_set master;
	FD_ZERO(&master);
	FD_SET(fd, &master);
	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);
	struct timeval timer;
	int result;
	int rcv;
	char* data = (char*)malloc((PAYLOAD + 5) * sizeof(char));
	char* ACK = (char*)malloc(5 * sizeof(char));
	for(;;){
		timer.tv_sec = 0;
		timer.tv_usec = TWAIT;
		readfds = master;
		result = select(fd+1, &readfds, NULL, NULL, &timer);
		switch(result){
		case 0: // time out and close socket
			if((rcv = close(fd))!=0){
				perror("rdt_close:");
				return -1;
			}else{
				return 0;
			}
		case -1:
			perror("select");
			exit(1);
		default:
			rcv = recv(fd, data, sizeof data, 0);
			if(rcv <= HEADER){ // ignore the ACK pkt
				printf("rdt_close: Receive a ACK and ignore it\n");
				continue;
			}else{ // if still receive pkt ACK it 
				printf("rdt_close: Receive a Packet and re-ACK it\n");
				make_ACK(nextseqnum - 1,ACK);
				udt_send(fd, ACK, HEADER , 0);
			}

		}
	}
}

#endif
