/********************************************************/
// Generic FIFO routines using POSIX shared memory		 
//                                                   
// Each FIFO buffer consists of a generic header for    
// handling the shared memory and basic FIFO functions  
// (i.e. head and tail pointers, base addresses, etc.), 
// a user header which describes something common about 
// the FIFO records (i.e. gate spacing, scaling factor, 
// etc.), and an array of FIFO records.			
//                                                      
// If the FIFO is full, the data just overwrites the    
// last FIFO record.					
//                                                      
// Shared memory is managed using a list which is also  
// shared memory. Programs can access the shared memory 
// containing this list by getting the address from a   
// file called /temp/sharedmem.list. This address needs 
// to be read once. Subsequent dynamic allocations of	
// shared memory FIFO buffers get reflected in the list 
// immediately and all programs have shared access to	
// that information.					
//                                                      
// The fifo users are awakened by UDP socket writes on up
// to N ports.
//
/********************************************************/

#include <Afx.h>

/* for shmem calls */
#include	<string.h>
#include	<stdio.h>

/* for sockets */
//#include <winsock.h>
#include <winsock2.h>

#include "../include/proto.h"

//#define	ERROR	-1

/* This shorthand is useful */
#define	FP	fp->header

/* Allocate memory for a new fifo buffer. */
/* If the FIFO is already open, then just use it */
/* This assumes that a FIFO with a specific name is always */
/* used the same way by any task (i.e. size, structure type, etc.) */
FIFO *fifo_create(char *name, int headersize, int recordsize, int recordnum)
   {
   int 		size;
   FIFO	*fifo;

//!!! mp 6-4-03:
//THIS CAUSES FIFO WRAPAROUND DATA CORRUPTION:
//   size = sizeof(FIFO) + headersize + recordsize * recordnum;
//!!!THIS WORKS:
   size = sizeof(FIFO) + headersize + recordsize * (recordnum+1);
    
   if(fifo_exhist(name))
      fifo = (FIFO *)shared_mem_open(name);
   else
      fifo = (FIFO *)shared_mem_create(name, size);

//    printf( "FIFO buffer named %s located at memory address 0x%08x\n", name, fifo );

   if(fifo)
      {
	   /* initialize the fifo structure */
	   strncpy(fifo->name,name,80);
	   fifo->header_off = sizeof(FIFO);		/* pointer to the user header */
	   fifo->fifobuf_off = fifo->header_off + headersize;	/* pointer to fifo base address */
	   fifo->record_size = recordsize;						/* size in bytes of each FIFO record */
	   fifo->record_num = recordnum;					/* number of records in FIFO buffer */
	   fifo->head = fifo->tail = 0;							/* indexes to the head and tail records */
	   fifo->destroy_flag = 0;								/* destroy-when-empty flag */
	   fifo->sock = -1;  /* indicating it's unitialized */
	   fifo->port = 20000;
	   fifo->clients = 0;
	   fifo->magic = MAGIC;  /* do this last so clients can use it as done flag */
      }
      
   return(fifo);
   }

/*  initialize piraq fifo: replaces fifo_create() call */ 
void piraq_fifo_init(FIFO * fifo, char *name, int headersize, int recordsize, int recordnum)
{
   if(fifo)
      {
	   /* initialize the fifo structure */
	   strncpy(fifo->name,name,80);
	   fifo->header_off = sizeof(FIFO);		/* pointer to the user header */
	   fifo->fifobuf_off = fifo->header_off + headersize;	/* pointer to fifo base address */
	   fifo->record_size = recordsize;						/* size in bytes of each FIFO record */
	   fifo->record_num = recordnum;					/* number of records in FIFO buffer */
	   fifo->head = fifo->tail = 0;							/* indexes to the head and tail records */
	   fifo->destroy_flag = 0;								/* destroy-when-empty flag */
	   fifo->sock = -1;  /* indicating it's unitialized */
	   fifo->port = 20000;
	   fifo->clients = 0;
	   fifo->magic = MAGIC;  /* do this last so clients can use it as done flag */
      }
// else here 
}
 
/* Get a pointer to the next available FIFO. */
/* Returns NULL if none available (already allocated */
/* by fifo_create). */
FIFO *fifo_open(char *name)
   {
   FIFO	*fifo;
   
   fifo = (FIFO *)shared_mem_open(name);
   return(fifo);
   }

/* do your best to destroy and remove FIFO from operating system */
int fifo_close(FIFO *fifo)
   {
   char	name[8];
   void	*addr;
   int		size;

   strcpy(name,fifo->name);
   addr = fifo;
   size = fifo->size;
   fifo->magic = 0;
   
   shared_mem_close(name,addr,size);
   return(0);
   }

/* Return a pointer to the next working write record. */
/* Return -1 if error */
void *fifo_get_write_address(FIFO *fifo)
   {
   return((char *)fifo + fifo->fifobuf_off + fifo->record_size * fifo->head); 
   }

/* reset the FIFO pointers so that the FIFO appears empty */
void fifo_clear(FIFO *fifo)
   {
   fifo->tail = fifo->head = 0;
   }

static int Last = 0;

/* Return a pointer to a good pointer near the head */
/* Return -1 if error */
void *fifo_get_last_address(FIFO *fifo)
   {
   Last = fifo->head - 1;
   if(Last < 0)	Last += fifo->record_num;
   return((char *)fifo + fifo->fifobuf_off + fifo->record_size * Last); 
   }

void *fifo_index_from_last(FIFO *fifo,int index)
{
	int i;

	i = (Last + index) % fifo->record_num; // nevermind
	if(i<0) i+= fifo->record_num;
	return((char *)fifo + fifo->fifobuf_off + fifo->record_size * i);
}


/* Increment the FIFO head pointer. Return the number of remaining */
/* records or -1 on error */
int fifo_increment_head(FIFO *fifo)
   {
   int	free;

   if(!fifo)	return(-1);

   /* this code throws away oldest data */
   fifo->head = (fifo->head + 1) % fifo->record_num;		   /* increment head */
   if(fifo->head == fifo->tail)   /* if you are about to overwrote some data */
      {
      printf("FIFO overflow (%s)\n",fifo->name);
      fifo->tail = (fifo->tail + 1) % fifo->record_num;	/* increment tail */
      }

   /* this code throws away newest data */
//   if( ((fifo->head + 1) % fifo->record_num) != fifo->tail) /* if not full */
//      fifo->head = (fifo->head + 1) % fifo->record_num;		   /* increment head */

//   fifo_notify(fifo);			/* notify the receiving routine of a new FIFO entry */

   /* return the number of remaining records */
   free = fifo->tail - fifo->head;
   if(free <= 0)	free += fifo->record_num;
   return(free);
   }

static	int	Count=0;
static	FIFORING	Pkt;
static  unsigned int notify_seq = 0; 

void fifo_notify(FIFO *fifo)
   {
   int	i;
   
   if(fifo->sock == -1)	return;		/* just to be sure */

   Pkt.udphdr.magic = MAGIC;
   Pkt.udphdr.type   = 0;
   Pkt.udphdr.totalsize = sizeof(int);

   Pkt.cnt = Count++;
   for(i=0; i<5; i++) { 
      send_udp_packet(fifo->sock, fifo->port + i, notify_seq, (UDPHEADER *)&Pkt); // notify_seq++; 
//current:      send_udp_packet(fifo->sock, fifo->port, notify_seq, (UDPHEADER *)&Pkt); // notify_seq++; 
	  }
notify_seq++; // here, or there? 
   }

/* Return a pointer to the next working read record. */
/* Return a NULL if the FIFO is empty */
/* Return -1 if error */
void *fifo_get_read_address(FIFO *fifo, int offset)
   {
   int	offptr;
   
   if(!fifo)	return((void *)0);

//   if(fifo->head == fifo->tail)	/* if empty */
//      return(NULL);		/* return NULL indicating empty */

   offptr = fifo->tail + offset;
   while(offptr < 0) offptr += fifo->record_num;

//(fifo->tail + offset) % fifo->record_num

   return((char *)fifo + fifo->fifobuf_off + fifo->record_size * offptr); 
   }

/* Increment the FIFO tail pointer. Return the number of used */
/* records or -1 on error */
int fifo_increment_tail(FIFO *fifo)
   {
   if(!fifo)	return(-1);

   if(fifo->head == fifo->tail)	/* if empty */
      return(0);		/* return without incrementing */

   fifo->tail = (fifo->tail + 1) % fifo->record_num;	/* increment tail */

   return(fifo_hit(fifo));
   }


/* Return number of records waiting to be read */
/* or -1 on error */
int fifo_hit(FIFO *fifo)
   {
   int	used;

   if(!fifo)	return(-1);

//   if(fifo->head == fifo->tail)	/* if empty */
//      if(fifo->destroy_flag)	/* if destroy when empty */
//	{
//	fifo_close(fifo->name);
//	return(-1);
//	}

   used = fifo->head - fifo->tail;
   if(used < 0)	used += fifo->record_num;
   return(used);
   }

int fifo_exhist(char *name)
   {
   return(shared_mem_exhist(name));
   }

void *fifo_get_header_address(FIFO *fifo)
   {
   return((char *)fifo + fifo->header_off);
   }