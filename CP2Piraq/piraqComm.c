/********************************************************/
// Generic circular buffer (cb) routines		 
//                                                   
// Each CB buffer consists of a generic header for    
// handling the basic cb functions  
// (i.e. head and tail pointers, base addresses, etc.), 
// a user header which describes something common about 
// the CB records (i.e. gate spacing, scaling factor, 
// etc.), and an array of CB records.			
//                                                      
// If the CB is full, the data just overwrites the    
// last CB record.					
//                                                      
/********************************************************/

/* for shmem calls */
#include	<string.h>
#include	<stdio.h>

#include "piraqComm.h"
#define	ERROR	-1

/* This shorthand is useful */
#define	FP	fp->header

/* Allocate memory for a new cb buffer. */
/* If the cb is already open, then just use it */
/* This assumes that a cb with a specific name is always */
/* used the same way by any task (i.e. size, structure type, etc.) */
CircularBuffer *cb_create(int headersize, int recordsize, int recordnum)
   {

   CircularBuffer	*cb;
   cb = (CircularBuffer *)PCIBASE;

   if(cb)
      {
	   /* initialize the cb structure */
	   cb->header_off = sizeof(CircularBuffer);		/* pointer to the user header */
	   cb->cbbuf_off = cb->header_off + headersize;	/* pointer to cb base address */
	   cb->record_size = recordsize;						/* size in bytes of each CircularBuffer record */
	   cb->record_num = recordnum;					/* number of records in CircularBuffer buffer */
	   cb->head = cb->tail = 0;							/* indexes to the head and tail records */
      }
      
   return(cb);
   }


/* Get a pointer to the next available CircularBuffer. */
/* Returns NULL if none available (already allocated */
/* by cb_create). */
CircularBuffer *cb_open()
   {
   CircularBuffer	*cb;
   cb = (CircularBuffer *)PCIBASE;
   return(cb);
   }

/* do your best to destroy and remove CircularBuffer from operating system */
int cb_close(CircularBuffer *cb)
   {
   return(0);
   }

/* Return a pointer to the next working write record. */
/* Return -1 if error */
void *cb_get_write_address(CircularBuffer *cb)
   {
   return((char *)cb + cb->cbbuf_off + cb->record_size * cb->head); 
   }


/* Return a pointer to a good pointer near the head */
/* Return -1 if error */
void *cb_get_last_address(CircularBuffer *cb)
   {
   int	last;
   
   last = cb->head - 1;
   if(last < 0)	last += cb->record_num;
   return((char *)cb + cb->cbbuf_off + cb->record_size * last); 
   }

/* Increment the CircularBuffer head pointer. Return the number of remaining */
/* records or -1 on error */
int cb_increment_head(CircularBuffer *cb)
   {
   int	free;

   if(!cb)	return(-1);

   /* this code throws away oldest data */
   cb->head = ((cb->head) + 1) % cb->record_num;		   /* increment head */
   if(cb->head == cb->tail)   /* if you are about to overwrote some data */
      {
      cb->tail = (cb->tail + 1) % cb->record_num;	/* increment tail */
      }

   /* return the number of remaining records */
   free = cb->tail - cb->head;
   if(free <= 0)	free += cb->record_num;
   return(free);
   }

/* Return a pointer to the next working read record. */
/* Return a NULL if the CircularBuffer is empty */
/* Return -1 if error */
void *cb_get_read_address(CircularBuffer *cb, int offset)
   {
   int	offptr;
   
   if(!cb)	return((void *)-1);

   offptr = cb->tail + offset;
   while(offptr < 0) offptr += cb->record_num;

   return((char *)cb + cb->cbbuf_off + cb->record_size * offptr); 
   }

/* Increment the CircularBuffer tail pointer. Return the number of used */
/* records or -1 on error */
int cb_increment_tail(CircularBuffer *cb)
   {
   if(!cb)	return(-1);

   if(cb->head == cb->tail)	/* if empty */
      return(0);		/* return without incrementing */

   cb->tail = (cb->tail + 1) % cb->record_num;	/* increment tail */

   return(cb_hit(cb));
   }

/* Return number of records waiting to be read */
/* or -1 on error */
int cb_hit(CircularBuffer *cb)
   {
   int	used;

   if(!cb)	return(-1);

   used = cb->head - cb->tail;
   if(used < 0)	used += cb->record_num;
   return(used);
   }

/* Return a pointer to the next working write record. */
/* Return -1 if error */
void *cb_get_header_address(CircularBuffer *cb)
   {
   return((char *)cb + cb->header_off); 
   }

