/****************************************/
/*		  	 CLUTTERFILTER.C          	*/
/*								 		*/
/* 		4 Pole Clutter Filter			*/
/*		   RAPIDOW_D Version			*/
/****************************************/

/*
Rapid DOW version

Incorporates pulse pair, clutter filter, etc. 

Compiler Directives:
	Define	CF_ON to force filter ON
	Define 	DEBUG_INTOUT	to generate an integer output ramp
	Define	DEBUG_INTMID	to put integers into input of pulsepair
	Define	DEBUG_INTIN		to generate an integer input ramp
	Define	DEBUG_TS	to jam known integer data into TS
	Define	REVB		to accomodate data in lower 18 bits,
						 otherwise data assumed in upper 18 bits (REVD+).	

Rev history:
1/2/03	PP code added to working single_p3iq version.
1/3/03	Moved fifo_increment_head inside output section
		to solve fifo hit proplem.  Added sign extension to 
		A/D input data.
1/6/03	Added REVD data FIFO format capability (data in 
		upper 18 bits of FIFO word)
1/27/03	Added clutter filter 
1/30/03	Data ingest, pulsepair, and clutter filter in separate loops
2/3/03	Data ingest, pulsepair, and clutter filter in separate subroutines.
3/5/03	Rewrote code on top of DMA code a la Eric.
3/31/03	Finally, with working board, the code works as advertised.
*/
//define OLD_WAY
extern 	float 	coeff[];
void clutterfilter(int ngates, float * restrict iqp, float * restrict dlyp)
	{
	register float *iqptr;
	register float x, y, w1, w2, z1, z2;

	int i;
	float * restrict dlyPtr_next, * restrict dlyPtr_prev, * restrict CFptr;
//	float *dlyPtr_next, *dlyPtr_prev, *CFptr;
		
	//  Copy pointers to previous and next IIR delay buffers
	dlyPtr_next = dlyPtr_prev = dlyp;

	// Copy pointer to input data
	iqptr = iqp;

#ifdef OLD_WAY	
	for(i=0; i<ngates; i++)
    	{
	  	// Set the coefficient pointer for channel 0
  		//  and run IIR filter on both I and Q
		CFptr = (float *)coeff;

		x  = *iqptr;
		y  = (x * *CFptr++) + *dlyPtr_prev++;
		w1 = (x * *CFptr++) + *dlyPtr_prev++;
		w2 = (x * *CFptr++);
		*dlyPtr_next++ = w1 - (y * *CFptr++);
		*dlyPtr_next++ = w2 - (y * *CFptr++);
		      
		x  = (y * *CFptr++) + *dlyPtr_prev++;
		w1 = (y * *CFptr++) + *dlyPtr_prev++;
		w2 = (y * *CFptr++);
		*dlyPtr_next++ = w1 - (            x  * *CFptr++);
		*dlyPtr_next++ = w2 - ((*iqptr++ = x) * *CFptr++);

		x  = *iqptr;
		y  = (x * *CFptr++) + *dlyPtr_prev++;
		w1 = (x * *CFptr++) + *dlyPtr_prev++;
		w2 = (x * *CFptr++);
		*dlyPtr_next++ = w1 - (y * *CFptr++);
		*dlyPtr_next++ = w2 - (y * *CFptr++);
	
		x  = (y * *CFptr++) + *dlyPtr_prev++;
		w1 = (y * *CFptr++) + *dlyPtr_prev++;
		w2 = (y * *CFptr++);
		*dlyPtr_next++ = w1 - (            x  * *CFptr++);
		*dlyPtr_next++ = w2 - ((*iqptr++ = x) * *CFptr  );
		}
		// Reset the coefficient pointer for channel 1
		//  and run the IIR filter on both I and Q
	for(i = 0; i < ngates; i++)
		{
		CFptr = (float *)coeff;	
		x  = *iqptr;
		y  = (x * *CFptr++) + *dlyPtr_prev++;
		w1 = (x * *CFptr++) + *dlyPtr_prev++;
		w2 = (x * *CFptr++);
		*dlyPtr_next++ = w1 - (y * *CFptr++);
		*dlyPtr_next++ = w2 - (y * *CFptr++);
		      
		x  = (y * *CFptr++) + *dlyPtr_prev++;
		w1 = (y * *CFptr++) + *dlyPtr_prev++;
		w2 = (y * *CFptr++);
		*dlyPtr_next++ = w1 - (            x  * *CFptr++);
		*dlyPtr_next++ = w2 - ((*iqptr++ = x) * *CFptr++);
	
		x  = *iqptr;
		y  = (x * *CFptr++) + *dlyPtr_prev++;
		w1 = (x * *CFptr++) + *dlyPtr_prev++;
		w2 = (x * *CFptr++);
		*dlyPtr_next++ = w1 - (y * *CFptr++);
		*dlyPtr_next++ = w2 - (y * *CFptr++);
	
		x  = (y * *CFptr++) + *dlyPtr_prev++;
		w1 = (y * *CFptr++) + *dlyPtr_prev++;
		w2 = (y * *CFptr++);
		*dlyPtr_next++ = w1 - (            x  * *CFptr++);
		*dlyPtr_next++ = w2 - ((*iqptr++ = x) * *CFptr  );
		}
#else

	for(i=0; i<4*ngates; i++)
    	{
	  	// Set the coefficient pointer for channel 0;
  		//  and run IIR filter on both I and Q
		CFptr = (float *)coeff;

		x  = *iqptr;
		y  = (x * *CFptr++) + *dlyPtr_prev++;
		w1 = (x * *CFptr++) + *dlyPtr_prev++;
		w2 = (x * *CFptr++);
		*dlyPtr_next++ = w1 - (y * *CFptr++);
		*dlyPtr_next++ = w2 - (y * *CFptr++);

		x = y;
		y  = (x * *CFptr++) + *dlyPtr_prev++;
		w1 = (x * *CFptr++) + *dlyPtr_prev++;
		w2 = (x * *CFptr++);
		*dlyPtr_next++ = w1 - (y * *CFptr++);
		*dlyPtr_next++ = w2 - (y * *CFptr++);
		*iqptr++ = y;		     
		}

/* test Channel1 only */
/*
	for(i=0; i<ngates; i++)
    	{
	  	// Set the coefficient pointer for channel 0;
  		//  and run IIR filter on both I and Q

		CFptr = (float *)coeff;
		iqptr += 2;
		dlyPtr_prev += 8;
		dlyPtr_next += 8;     
		x  = *iqptr;
		z1 = *dlyPtr_prev++;
		z2 = *dlyPtr_prev++;
		y  = (x * *CFptr++) + z1;
		w1 = (x * *CFptr++) + z2;
		w2 = (x * *CFptr++);
		*dlyPtr_next++ = w1 - (y * *CFptr++);
		*dlyPtr_next++ = w2 - (y * *CFptr++);

		x = y;
		z1 = *dlyPtr_prev++;
		z2 = *dlyPtr_prev++;
		y  = (x * *CFptr++) + z1;
		w1 = (x * *CFptr++) + z2;
		w2 = (x * *CFptr++);
		*dlyPtr_next++ = w1 - (y * *CFptr++);
		*dlyPtr_next++ = w2 - (y * *CFptr++);
		*iqptr++ = y;	     

		x  = *iqptr;
		z1 = *dlyPtr_prev++;
		z2 = *dlyPtr_prev++;
		y  = (x * *CFptr++) + z1;
		w1 = (x * *CFptr++) + z2;
		w2 = (x * *CFptr++);
		*dlyPtr_next++ = w1 - (y * *CFptr++);
		*dlyPtr_next++ = w2 - (y * *CFptr++);

		x = y;
		z1 = *dlyPtr_prev++;
		z2 = *dlyPtr_prev++;
		y  = (x * *CFptr++) + z1;
		w1 = (x * *CFptr++) + z2;
		w2 = (x * *CFptr++);
		*dlyPtr_next++ = w1 - (y * *CFptr++);
		*dlyPtr_next++ = w2 - (y * *CFptr++);
		*iqptr++ = y;

		}		
*/		
#endif
	return;
	}

  
   


