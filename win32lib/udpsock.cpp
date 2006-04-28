#include <Afx.h>
//#include <stdafx.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <winsock.h>
#include <winsock2.h>
//#include <Ws2spi.h>
#include <sys/types.h>

#include	"../include/proto.h"

#define	MAXFD	9999

static	int	Inet_addr[MAXFD];  // remember the Inet_addr for each file descriptor 
static int Lastpage[MAXFD];

static struct sockaddr_in  SockAddr;

static	int	FIRSTIME = 1;
static char *readbuf;

int	wastime(int in)  {return in;}

 // this is the guy who usually transmits 
 // this is the server 
#ifdef	STREAM_SOCKET
void open_udp_out(char *name)	//	Stream implementation
   {
  // Initialize Winsock
  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != NO_ERROR)
    printf("Error at WSAStartup()\n");

  //----------------------
  // Create a SOCKET for connecting to server
  SOCKET ConnectSocket;
  ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (ConnectSocket == INVALID_SOCKET) {
    printf("Error at socket(): %ld\n", WSAGetLastError());
    WSACleanup();
    return;
  }

  //----------------------
  // The sockaddr_in structure specifies the address family,
  // IP address, and port of the server to be connected to.
  sockaddr_in clientService; 
  clientService.sin_family = AF_INET;
  clientService.sin_addr.s_addr = inet_addr( "128.117.85.189" );
  clientService.sin_port = htons( 27015 );

  //----------------------
  // Connect to server. 
  if ( connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) ) == SOCKET_ERROR) {
    printf( "Failed to connect. sizeof(clientService) = %d\n", sizeof(clientService) );
int iError = WSAGetLastError ();
if (iError) { 
	printf("connect(): iError = 0x%x\n",iError);
}
    WSACleanup();
    return;
  }

  //----------------------
  // Declare and initialize variables.
  int bytesSent;
  int bytesRecv = 0;
  char sendbuf[32] = "Client: Sending data.";
  char recvbuf[32] = "";

  //----------------------
  // Send and receive data.
  bytesSent = send( ConnectSocket, sendbuf, strlen(sendbuf), 0 );
  printf( "Bytes Sent: %ld\n", bytesSent );

  while( bytesRecv != SOCKET_ERROR ) {
    bytesRecv = recv( ConnectSocket, recvbuf, 32, 0 );
    if ( bytesRecv == 0 ) {
      printf( "Connection Closed.\n");
      break;
    }
    printf( "Bytes Recv: %ld\n", bytesRecv );
  }

  if ( bytesRecv == SOCKET_ERROR )
  {
     printf("recv failed: %d\n", WSAGetLastError());
  }

  WSACleanup();
  return;
   }
#else	//	UDP implementation: previous method
int open_udp_out(char *name)	//	Datagram implementation
   {
	int	sock,i,error;
	struct hostent* hent;
    WSADATA w;

   if(FIRSTIME)	// initialize the Lastpage array exactly once 
      {
      for(i=0; i<MAXFD; i++)
        Lastpage[i] = -1;
		FIRSTIME = 0;

		// Must be done at the beginning of every WinSock program
		error = WSAStartup (0x0202, &w);   // Fill in w

		if (error)
		{ // there was an error
			printf("!!!WSAStartup(): error = -1\n");
		  return -1;
		}
		if (w.wVersion != 0x0202)
		{ // wrong WinSock version!
		  WSACleanup (); // unload ws2_32.dll
			printf("!!!WSACleanup(): error = -1\n");
		  return -1;
		}
      }

    // set up the socket stuff  
   memset(&SockAddr, 0, sizeof(struct sockaddr_in));

   //if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == ERROR)  return(-1);  // get a UDP socket 
   sock = WSASocket(AF_INET,SOCK_DGRAM,IPPROTO_UDP,NULL,0,WSA_FLAG_OVERLAPPED);	//	was dwFlags = 0
int iError;
if (1) { 
	iError = WSAGetLastError ();
	printf("!!!WSASocket(): iError = 0x%x\n",iError);
}
	//int bufLen = 1024*1024*2;
	//int size = 4;
	//setsockopt(sock,SOL_SOCKET,SO_SNDBUF,"100000",strlen("100000"));

//   Inet_addr[sock] = inet_addr("127.0.0.1");
   
   hent = gethostbyname(name);
 
   Inet_addr[sock] = *(long*)hent->h_addr;
 
   if(Inet_addr[sock] == ERROR)  return(-1);

   return(sock);
   }
#endif
   
//???static unsigned int sequence_num = 0;  

// this assumes the buffer is preceeded by a buffer header structure
// return value current sequence# 
unsigned int send_udp_packet(int sock, int port, unsigned int sequence_num, UDPHEADER *udp)
   {
   int		remain,size;
   int      r_c; // sendto() return code 
   UDPHEADER *next;

   if(udp->magic != MAGIC)
      {printf("send_udp_packet: !!Failed to send udp packet. Improper header\n");	return(-1);}  // something is wrong, just return 
   
   SockAddr.sin_port = htons(port);
   SockAddr.sin_addr.s_addr = Inet_addr[sock];
   SockAddr.sin_family = AF_INET;

   udp->pagenum = 0;
   udp->pages = udp->totalsize / MAXPACKET + 1;
   
   WSAOVERLAPPED Overlapped;
   DWORD dwFlags;
   DWORD lBytesSent = 0;
   int iBuf=0;

   WSABUF DataBuf[1];
   DataBuf[0].buf = (char *)udp;
   DataBuf[0].len = udp->totalsize; // remove +sizeof(UDPHEADER) 4-19-06, mp
   int iError = WSASendTo(sock,&DataBuf[0],1,&lBytesSent,0,(sockaddr *)&SockAddr,sizeof(SockAddr),NULL,NULL);
#if 1
//  if ((sequence_num % 100) == 0) {
   char buf[255];
   if (iError != 0) { // send error, print it
      printf("Error:%d, Bytes: %d ",iError,lBytesSent);
      printf("totalsize:%d\n",udp->totalsize);
      if (iError) { 
	     iError = WSAGetLastError ();
	     printf("WSASendTo(): iError = 0x%x\n",iError);
      }
   }
//  }
#endif
   sequence_num+=udp->pages;
   return(sequence_num);

   }
   
 //returns a 1 when the last page arrives 
 //returns a -1 on error 
 //zero otherwise 
 //maxbufsize is a not-to-exceed size for the buffer 
static int Num=0;
int receive_udp_packet(int sock, UDPHEADER *udp, int maxbufsize)
   {
	PACKET * whatsit; 
   UDPHEADER	*current,save;
   int		i;
char buffer[255]; // for test data
	if(readbuf == NULL)
		readbuf = new char[1024*1024];

   DWORD lNumBytes=0;
   DWORD lFlags=0;
   WSABUF buf;
   buf.buf = (char *)udp;
   buf.len = maxbufsize+sizeof(UDPHEADER);

   // Read in the packet
   int iRet = WSARecv(sock,&buf,1,&lNumBytes,&lFlags,NULL,NULL);
//?readbuf?   current = (UDPHEADER *)(readbuf);
   current = (UDPHEADER *)(udp);
//whatsit = (PACKET *)(readbuf);
//sprintf(buffer,"&curr: 0x%x &curr->totalsize: 0x%x", current, current->totalsize);
//sprintf(buffer,"iRet: %d", iRet);
//sprintf(buffer,"w->data...BN = %I64d", whatsit->data.info.beam_num);
//MessageBox(NULL,buffer,NULL,MB_OK);

   if(current->magic != MAGIC)
	  {
		  printf("Magic number wrong: %08X instead of 12345678\n",current->magic);
		  Lastpage[sock] = -1;
		  return(-3);
	  }

   return(1);

   // Point to the UDP structure
   int iNumPages = current->pages;
   int iPageSize = current->pagesize;
   int iPacketSize = sizeof(PACKETHEADER);
   memcpy((char *)udp,readbuf,iPageSize);
   for(i=1;i<iNumPages;i++)
   {
	  
	  int idxOffset = i * (MAXPACKET)+sizeof(UDPHEADER);
	  UDPPACKET *pkt = (UDPPACKET *)(readbuf+iPageSize*i);
	  //memset(pkt->buf,128,MAXPACKET);
	  memcpy((char *)udp + idxOffset,pkt->buf,MAXPACKET);
   }


	return(1);
 }

/*
 //returns a 1 when the last page arrives 
 //returns a -1 on error 
 //zero otherwise 
 //maxbufsize is a not-to-exceed size for the buffer 
int receive_udp_packet(int sock, UDPHEADER *udp, int maxbufsize)
   {
   UDPHEADER	*current,save;
   int		i;

   i = ++Lastpage[sock] * MAXPACKET;

//printf("Lastpage = %d\n",Lastpage[sock]);

   if(i > maxbufsize) // test for buffer overflow 
      {
      printf("read overflow\n");
      Lastpage[sock] = -1;	// start over 
      return(-1);
      }

   current = (UDPHEADER *)((char *)udp + i);		// point to the current read address

   // save valid data at end of last page 7
   if(Lastpage[sock] > 0) memcpy(&save,current,sizeof(UDPHEADER));

   recvfrom(sock, (char *)current, maxbufsize+sizeof(UDPHEADER),0,NULL,0);

   if(current->magic != MAGIC || current->pagenum != Lastpage[sock])
      {
      if(current->magic != MAGIC)
		  return(-3);
	  Lastpage[sock] = -1;
      return(-2);
      }  // something is wrong, just return 7

//printf("number of pages = %d\n",current->pages);

   if(Lastpage[sock] > 0)
      {
      if(Lastpage[sock] >= current->pages - 1)   Lastpage[sock] = -1;

      //restore valid data at end of last page 
//printf("restoring saved data\n");
      memcpy(current,&save,sizeof(UDPHEADER));
      }
   else
      if(Lastpage[sock] >= current->pages - 1)    Lastpage[sock] = -1;

   return(Lastpage[sock] == -1 ? 1 : 0);
   }*/
   
// this is the guy who usually receives 
// this is the client 
int open_udp_in(int port)
   {
   int i,rtn,error;
   struct sockaddr_in	insock;
   SOCKET sock;
   WSADATA w;

   if(FIRSTIME)	// Initialize once
      {
      for(i=0; i<MAXFD; i++)
         Lastpage[i] = -1;
	  
		// Must be done at the beginning of every WinSock program
		error = WSAStartup (0x0202, &w);   // Fill in w

		if (error)
		{ // there was an error
		  return -1;
		}
		if (w.wVersion != 0x0202)
		{ // wrong WinSock version!
		  WSACleanup (); // unload ws2_32.dll
		  return -1;
		}

		FIRSTIME = 0;
      }

   memset(&insock, 0, sizeof(struct sockaddr_in));

   if((sock = socket (AF_INET, SOCK_DGRAM, 0)) == ERROR) return(-1);  // get a UDP socket 

   insock.sin_family = AF_INET;

   // try to bind until you find an open port 
   for(i=0,rtn=ERROR; (i<5) && (rtn == ERROR); i++)
      {
      insock.sin_port  = htons(port+i);
      rtn = bind(sock, (struct sockaddr *)&insock, sizeof(struct sockaddr_in));
      }
   
   if(i >= 5 || rtn == ERROR) 
      {
		(void) closesocket(sock);
		printf("bind problem\n");
		return(-1);
      }

   //fromlen = sizeof(struct sockaddr_in);
   //if(getsockname(sock,(struct sockaddr *)&insock,&fromlen) == ERROR)
   //   {
	//	(void) closesocket(sock);
	//	printf("getsockname problem: %d \n",WSAGetLastError());
	//	return(-1);
   //   }

   return(sock);
   }

void close_udp(int sock)
{
	closesocket(sock);
	WSACleanup();
}