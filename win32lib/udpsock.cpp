#include <Afx.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <winsock.h>
#include <winsock2.h>
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
int open_udp_out(char *name)
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
		  return -1;
		}
		if (w.wVersion != 0x0202)
		{ // wrong WinSock version!
		  WSACleanup (); // unload ws2_32.dll
		  return -1;
		}
      }

    // set up the socket stuff 
   memset(&SockAddr, 0, sizeof(struct sockaddr_in));

   //if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == ERROR)  return(-1);  // get a UDP socket 
   sock = WSASocket(AF_INET,SOCK_DGRAM,IPPROTO_UDP,NULL,0,0);
	//int bufLen = 1024*1024*2;
	//int size = 4;
	//setsockopt(sock,SOL_SOCKET,SO_SNDBUF,"100000",strlen("100000"));

//   Inet_addr[sock] = inet_addr("127.0.0.1");
   
   hent = gethostbyname(name);
 
   Inet_addr[sock] = *(long*)hent->h_addr;
 
   if(Inet_addr[sock] == ERROR)  return(-1);

   return(sock);
   }
   
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
//   WSABUF DataBufs[50]; // why?
//   ZeroMemory(DataBufs,sizeof(WSABUF)*50);
   int iBuf=0;

   WSABUF DataBuf;
   DataBuf.buf = (char *)udp;
   DataBuf.len = udp->totalsize+sizeof(UDPHEADER); // add sizeof(UDPHEADER) 4-15-05, mp
   int iError = WSASendTo(sock,&DataBuf,1,&lBytesSent,0,(sockaddr *)&SockAddr,sizeof(SockAddr),NULL,NULL);
#if 0
char buf[255];
if (iError != 0) { 
sprintf(buf,"Error:%d, Bytes: %d",iError,lBytesSent);
sprintf(buf,"totalsize:%d",udp->totalsize);
MessageBox(NULL,buf,NULL,MB_OK);
}  
#endif
   sequence_num+=udp->pages;
   return(sequence_num);

   for(remain=udp->totalsize; remain>0; remain-=size)
      {
      // send the first page 
      size = (remain > MAXPACKET) ? MAXPACKET : remain; // data, NOT INCLUDING the header 
      udp->pagesize = size + sizeof(UDPHEADER);
	  udp->sequence_num = sequence_num; sequence_num++; 
	  
	  char *buf = new char[MAXPACKET + sizeof(UDPHEADER)];
	  memcpy(buf,udp,MAXPACKET + sizeof(UDPHEADER));
//not used:	  DataBufs[iBuf].len = udp->pagesize;
//	  DataBufs[iBuf].buf = buf;//(char *)udp;
 	  iBuf++;

	       //r_c = sendto(sock,(const char *)udp, udp->pagesize,0,(struct sockaddr *)&SockAddr,sizeof(SockAddr));
//	  if (r_c != udp->pagesize/* && (WSAGetLastError() == WSAENOBUFS)*/) {
//			Sleep(1000); //printf(""); 
//           r_c = sendto(sock,(const char *)udp, udp->pagesize,0,(struct sockaddr *)&SockAddr,sizeof(SockAddr));
//	  }	  
//#if 0
//	  while (r_c == SOCKET_ERROR/* && (WSAGetLastError() == WSAENOBUFS)*/) {
//			Sleep(1000); //printf(""); 
//            r_c = sendto(sock,(const char *)udp, udp->pagesize,0,(struct sockaddr *)&SockAddr,sizeof(SockAddr));
//	  }	  
//#endif
	  //Sleep(0);
	  //usleep(1);
//printf("sending page %d   size = %d  pagesize = %d remain = %d\n",udp->pagenum,size,udp->pagesize,remain);

//for(i=0; i<1000; i++) wastime(i);
#if 1
      if(remain > size)
         {
      // fill in the next header (destroys contents of buffer!) 
      next = (UDPHEADER *)((char *)udp + size);	// point to where the next header starts 
      next->magic = udp->magic;
      next->type = udp->type;
	  next->totalsize = udp->totalsize;
      next->pagenum = udp->pagenum+1;
      next->pages = udp->pages;
      udp = next;
         }
#endif
      }
		
   	  lBytesSent = 0;
	  ZeroMemory(&Overlapped,sizeof(WSAOVERLAPPED));
	  Overlapped.hEvent = NULL;
	  //WSASendTo(sock,DataBufs,iBuf,&lBytesSent,0,(struct sockaddr *)&SockAddr,sizeof(SockAddr),NULL,NULL);
//not used:		for(int i=0;i<iBuf;i++)
//			delete(DataBufs[i].buf);
      
	  //if(lBytesSent != udp->pagesize)
	//	  MessageBox(NULL,"BUNGLE!",NULL,MB_OK);

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