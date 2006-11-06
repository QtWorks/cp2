/****************************************/
/*		  	 PULSEPAIR.C             	*/
/*								 		*/
/* 		Pulse-pair routine				*/
/*		   RAPIDOW_D Version			*/
/****************************************/


#include 	"proto.h"

// RapidDOW version

//DEVELOP: Establish two abpp's to split channels for FIFO output   
// Pulse pair algorithm, single lag, single PRT

void  pulsepair(int ngates, float * restrict iqpp, float * restrict lagpp, float *abpp, int samplect)
  {
  int i;
  register float i0, q0, i1, q1;
  register float i0lag1, q0lag1, i1lag1, q1lag1;
  float *abp, localnorm;
  float *iqp, *lagp;
  		
  // Copy the pointers
  abp = abpp; iqp = iqpp; lagp = lagpp;

//#ifdef	CP2					// CP2 processor
//		return;				// CP2: eliminate pp calculation load
//#endif
#pragma MUST_ITERATE(8, ,2) 		
  	for(i = 0; i < ngates; i++)
		{	  	

		// Pick up latest I's and Q's
		i0 = *iqp++;
		q0 = *iqp++;
		i1 = *iqp++;
		q1 = *iqp++;

		// Pick up lag1 and replace with current value
		i0lag1 = *lagp;
		*lagp++ = i0;
		q0lag1 = *lagp;
		*lagp++ = q0;
		i1lag1 = *lagp;
 		*lagp++ = i1;
		q1lag1 = *lagp;
		*lagp++ = q1;

		// Compute complex products and add to running sums
      	// Chan 0
      	*abp++ += i0*i0lag1 + q0*q0lag1;	// Compute real part (A)
      	*abp++ += i0*q0lag1 - q0*i0lag1;	// Compute imag part (B)
      	*abp++ += i0*i0 + q0*q0;			// Compute magnitude (P)

   		// Chan 1
   		*abp++ += i1*i1lag1 + q1*q1lag1;	// Same for other channel
   		*abp++ += i1*q1lag1 - q1*i1lag1;
   		*abp++ += i1*i1 + q1*q1;
		}
  return;
  }



  
   


