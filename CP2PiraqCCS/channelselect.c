/********************************************/
/*		  	channelselect.c	           		*/
/*									 		*/
/* 	select 1 of 2 channels, gate-by-gate	*/
/*			CP2_D Version					*/
/********************************************/


#include 	"proto.h"

// CP2_D version

// specific function tbd; procedure tests 2-to-1 channel conversion with test data. mp 8-19-05. 
extern int m;

void  channelselect(int ngates, float * restrict src, float * restrict dst, unsigned int channelMode)
{
	int i;
	register float i0, q0, i1, q1;
	float *iqp;
	float * iqsrc, * iqdst; 
	
	iqsrc = src; iqdst = dst; 

//#pragma MUST_ITERATE(8, ,2) 		
	if	(channelMode == SEND_CHA)	{	// CHA selected
#pragma MUST_ITERATE(8, ,2) //	dup'd because "MUST_ITERATE pragma must immediately precede for/while/do-while"		
	  	for(i = 0; i < ngates; i++)	{	// all gates 
#if 0	//	send test data 
	  	if (i == 0) {
  			*iqdst++ = (float)m; 
			iqsrc++; 
  			*iqdst++ = *iqsrc++; iqsrc++; iqsrc++; // copy src CH1 to dst; drop CH2 data	
		}
		else {
  			*iqdst++ = *iqsrc++; 
  			*iqdst++ = *iqsrc++; iqsrc++; iqsrc++; // copy src CH1 to dst; drop CH2 data	
		}
#else	//	as it is:
  			*iqdst++ = *iqsrc++; *iqdst++ = *iqsrc++; iqsrc++; iqsrc++; // copy src CH1 to dst; drop CH2 data	
#endif
		}
	}
	else if	(channelMode == SEND_CHB)	{	// CHB selected
#pragma MUST_ITERATE(8, ,2) 
	  	for(i = 0; i < ngates; i++)	{	// all gates  
  			iqsrc++; iqsrc++; *iqdst++ = *iqsrc++; *iqdst++ = *iqsrc++; // copy src CH2 to dst; drop CH1 data	
		}
	}
	else	{	//	SEND_COMBINED: apply dynamic range extension algorithm here. 
				//	until defined SEND_CHA 
#pragma MUST_ITERATE(8, ,2) 
	  	for(i = 0; i < ngates; i++)	{	// all gates  
  			*iqdst++ = *iqsrc++; *iqdst++ = *iqsrc++; iqsrc++; iqsrc++; // copy src CH1 to dst; drop CH2 data	
		}
	}
}



  
   


