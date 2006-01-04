#include <Afx.h>

#include        <math.h>
#include        <stdio.h>
#include        <time.h>
#include        <string.h>
#include        <stdlib.h>

#include        "../include/proto.h"
#include		<windows.h>

#define			FFT_NOISE	0	/* Refine this noise floor later GRG */

/* GRG 10/12/01 Modified dsp_simplepp and pulsepair to effectively clear ABP buffer
without directly writing 0's */
/* process a dwell worth of data */
/* store results in SAMPLER structure */
/* on entry, pulsenum points to the begining of the dwell to be processed */
void dsp(DATABLOCK *abpdata, FIFO *fifo)
   {
   //int          i,j,end,vend;
   //short        *iqs,ii,qq;
   //float        *a,*iqptr,*lagbuf,*ts,*iqf,*delaysave;
   //UINT64 pulsenum;
	int i=0;
		
	switch(abpdata->info.dataformat)
      {
      case DATA_SIMPLEPP: dsp_simplepp(abpdata,fifo);	break;
      case DATA_POLYPP  :
      case DATA_DUALPP  :
      case DATA_POL1    :
      case DATA_POL2    : 
      case DATA_POL3    : break;
      case DATA_SIMPLE16: dsp_simple16(abpdata,fifo);	break;
      case DATA_DOW     :
      case DATA_FULLPOL1:
      case DATA_FULLPOLP:
      case DATA_MAXPOL  :	break;
      case DATA_HVSIMUL :	
      case DATA_SHRTPUL :	break;
      case DATA_SMHVSIM :	dsp_hvsimul(abpdata,fifo);  break;
 	  case DATA_FFT		: dsp_simplefft(abpdata, fifo);	break; /* V, blank, P out */
	  case DATA_STOKES	: dsp_stokes(abpdata, fifo);	break; /* I1, I2, U, V out */
//case DATA_TIMESERIES: break; //!!!
     default:			break;
      }

   }
/*
Added Stokes parameter computations 
GRG 10/14/01
*/

void dsp_stokes(DATABLOCK *abpdata, FIFO *fifo)
{
unsigned int i;
float *iqptr, *databuf;
float Knorm;

databuf = abpdata->data;	/* Hang on to initial output data pointer for later normalization */

Knorm = 1.0/(float)abpdata->info.hits;	/* Normalization number for stokes computation */

for(i = 0; i<abpdata->info.hits; i++)
	{
	iqptr  = ((PACKET *)fifo_get_read_address(fifo,1+i-abpdata->info.hits))->data.data;     /* work your way around this pair of buffers */
    stokes(i, abpdata->info.gates, (short *)iqptr, Knorm, abpdata->data);		/* Run the stokes algorithm for all gates */
	}

/* Run through the output data and normalize */

for(i = 0;i < 4*abpdata->info.hits; i++)	
	{
	*databuf++ *= Knorm;
	}
return;
}

/* 
Added fft computation of mean velocity and signal power.
The output format is the familiar ABP, 
where:
	A is NCP*sin(k*v),
	B is NCP*cos(k*v),
	k is Pi/Vnyquist, and
	P is signal power.
10/14/01 GRG
*/
void dsp_simplefft(DATABLOCK *abpdata, FIFO *fifo)
{
unsigned int i, j;
float  *outbuf;
complex cgate[512];

outbuf = abpdata->data;

/* Collect one gate's worth of I and Q */

/* ADDRESSING QUESTION - GATES vs HITS? */
for(j = 0; j < abpdata->info.gates; j++)
	{
	for(i = 0; i<abpdata->info.hits; i++)
		{
		cgate[i].x = ((PACKET *)fifo_get_read_address(fifo,i-abpdata->info.hits))->data.data[4*j];     /* work your way around this pair of buffers */
		cgate[i].y = ((PACKET *)fifo_get_read_address(fifo,i-abpdata->info.hits))->data.data[4*j+1];     /* work your way around this pair of buffers */
		}
	/* Compute the first few moments from FFT */
	fftmoments(cgate, abpdata, outbuf);	
	}
}


void dsp_simplepp(DATABLOCK *abpdata, FIFO *fifo)
   {
   unsigned int  i;
   float        *iqptr,*lagbuf;
	/* Initial clearing now done in simplepp GRG */
      /* compute abp from IQ's stored in IQfifo */
   /* there is at least hits beams in the fifo by assumption */
   for(i=0; i<abpdata->info.hits; i++)  /* repeat for a dwell */
      {
      iqptr  = ((PACKET *)fifo_get_read_address(fifo,1+i-abpdata->info.hits))->data.data;     /* work your way around this pair of buffers */
      lagbuf = ((PACKET *)fifo_get_read_address(fifo,  i-abpdata->info.hits))->data.data;     /* work your way around this pair of buffers */
/*
GRG Included index in call to pulsepair.
It would be more efficient to have a simppstart routine to effectively clear ABP 
*/
      pulsepair(i, abpdata->info.gates,(short*)iqptr,(short*)lagbuf,abpdata->data);        /* compute A,B, and P */
      }
   }

void dsp_simple16(DATABLOCK *abpdata, FIFO *fifo)
   {
   unsigned int          i,end;
   float        *aptr,*iqptr,*lagbuf;

   float scale =  10.0 / log(10.0) / .004;  /* approx = 1085.7 */
   float hscale = 0.5 * scale;
   float offset = - 2.0 * scale * log(32768.0 / (double)0x10000)
	   - scale * log((double)abpdata->info.hits);
   float velfactor = 65536.0 / TWOPI;

   /* clear the abp ram */
   aptr = abpdata->data;
   end = 3 * abpdata->info.gates;
   for(i=0; i<end; i++)        *aptr++ = 0.0;

   /* compute abp from IQ's stored in IQfifo */
   /* there is at least hits beams in the fifo by assumption */
   for(i=0; i<abpdata->info.hits; i++)  /* repeat for a dwell */
      {
      iqptr  = ((PACKET *)fifo_get_read_address(fifo,1+i-abpdata->info.hits))->data.data;     /* work your way around this pair of buffers */
      lagbuf = ((PACKET *)fifo_get_read_address(fifo,  i-abpdata->info.hits))->data.data;     /* work your way around this pair of buffers */

      pulsepair(i, abpdata->info.gates,(short*)iqptr,(short*)lagbuf,abpdata->data);        /* compute A,B, and P */
      }

   float *fptr = abpdata->data;
   short *sptr = (short *)abpdata->data;

   // convert to 16 bit integer representation
   for(i=0; i<abpdata->info.gates; i++)
   {
		
	float a=*fptr++,b=*fptr++,p=*fptr++;
	if(a != 0.0 || b != 0.0)      
		{      
		*sptr++ = hscale * log(a*a+b*b) + offset;
		*sptr++ = atan2(b,a) * velfactor;
		}
	else      
		{      
		*sptr++ = 0;
		*sptr++ = 0;
		}
	*sptr++ =  scale * log(p) + offset;
   }

 }


/* do magnetron phase correction (in place) */
void correctphase(int gates, float *iqptr)
   {
   int          i;
   float        I,Q,I0,Q0,invmag;
   
   /* normalize and conjugate gate zero */
   I0 = iqptr[0];
   Q0 = iqptr[1];
   if(I0 == 0.0 && Q0 == 0.0)  invmag = 1.0; else
   invmag = (float)(1.0 / sqrt(I0 * I0 + Q0 * Q0));
   I0 *=  invmag;
   Q0 *= -invmag;
   
   /* correct all but the zeroeth of the gates with m and n */
   iqptr += 2;
   for(i=1; i<gates; i++)
      {
      I = *iqptr;
      Q = *(iqptr+1);
      *iqptr++ = I0 * I - Q0 * Q;
      *iqptr++ = Q0 * I + I0 * Q;
      }
   }

/* This Module performs a 4 Pole clutter filter on the Binet i, and q
   data buffers. The filter coefficients and gain can be modified to
   suit the application. The filter consists of two cascaded second order
   biquad IIR stages in Transpose form.
   The delay_ptr array is 8X the number of gates */
void c_filter(int gates, float *coeff, float *delay_ptr, float *iqptr)
   {
   float        *cfPtr,*dlyPtr_prev,*dlyPtr_next;
   float        w1,w2,x,y;
   int          i;

   dlyPtr_prev = delay_ptr;     /* size of array = GATES * 8 */
   dlyPtr_next = delay_ptr;

   for(i=0; i<gates; i++)
      {
      cfPtr = coeff;
	      
      x  = *iqptr;
      y  = (x * *cfPtr++) + *dlyPtr_prev++;
      w1 = (x * *cfPtr++) + *dlyPtr_prev++;
      w2 = (x * *cfPtr++);
      *dlyPtr_next++ = w1 - (y * *cfPtr++);
      *dlyPtr_next++ = w2 - (y * *cfPtr++);
	      
      x  = (y * *cfPtr++) + *dlyPtr_prev++;
      w1 = (y * *cfPtr++) + *dlyPtr_prev++;
      w2 = (y * *cfPtr++);
      *dlyPtr_next++ = w1 - (            x  * *cfPtr++);
      *dlyPtr_next++ = w2 - ((*iqptr++ = x) * *cfPtr++);

      x  = *iqptr;
      y  = (x * *cfPtr++) + *dlyPtr_prev++;
      w1 = (x * *cfPtr++) + *dlyPtr_prev++;
      w2 = (x * *cfPtr++);
      *dlyPtr_next++ = w1 - (y * *cfPtr++);
      *dlyPtr_next++ = w2 - (y * *cfPtr++);

      x  = (y * *cfPtr++) + *dlyPtr_prev++;
      w1 = (y * *cfPtr++) + *dlyPtr_prev++;
      w2 = (y * *cfPtr++);
      *dlyPtr_next++ = w1 - (            x  * *cfPtr++);
      *dlyPtr_next++ = w2 - ((*iqptr++ = x) * *cfPtr  );
      }
   }

/* accumulates into the abp buffer given a current and previous hit */
/* The lagbuf is 2X the number of gates */
void pulsepair(int indx, int gates,short *iqptr,short *lagbuf,float *abp)
   {
   int         i;
   float       Iold,Qold,I,Q;
   
   /* do pulse pair on a hit */
   for(i=0; i<gates; i++)
      {
      Iold = *lagbuf++;   /* read in an I */
      I = *iqptr++;

      Qold = *lagbuf++;   /* read in a Q */      
      Q = *iqptr++;

lagbuf+=2;
iqptr+=2;  // compute simple pulse pair from dual pol IQ's
		if(indx)
			{
			/* GRG do a single pulse pair (write over last accumulation) */
			*abp++ +=  Iold * I    + Qold * Q;        /* do a */
			*abp++ +=  Iold * Q    - Qold * I;        /* do b */
			*abp++ +=  I    * I    + Q    * Q;        /* do p */
			}
		else
			{
			/* GRG Do it without the addition (effectively zeros the array) */
			*abp++ =  Iold * I    + Qold * Q;        /* do a */
			*abp++ =  Iold * Q    - Qold * I;        /* do b */
			*abp++ =  I    * I    + Q    * Q;        /* do p */
			}
      }	
   }   

void dsp_hvsimul(DATABLOCK *abpdata, FIFO *fifo)
   {
   unsigned int i,end;
   float        *aptr,*iqptr,*lagbuf;

   float scale =  10.0 / log(10.0) / .004;  /* approx = 1085.7 */
   float hscale = 0.5 * scale;
   float offset = - 2.0 * scale * log(32768.0 / (double)0x10000)
	   - scale * log((double)abpdata->info.hits);
   float velfactor = 65536.0 / TWOPI;

   /* clear the abp ram */
   aptr = abpdata->data;
   end = 3 * abpdata->info.gates;
   for(i=0; i<end; i++)        *aptr++ = 0.0;

   /* compute abp from IQ's stored in IQfifo */
   /* there is at least hits beams in the fifo by assumption */
   for(i=0; i<abpdata->info.hits; i++)  /* repeat for a dwell */
      {
      iqptr  = ((PACKET *)fifo_get_read_address(fifo,1+i-abpdata->info.hits))->data.data;     /* work your way around this pair of buffers */
      lagbuf = ((PACKET *)fifo_get_read_address(fifo,  i-abpdata->info.hits))->data.data;     /* work your way around this pair of buffers */

      hvpulsep(abpdata->info.gates,(short*)iqptr,(short*)lagbuf,abpdata->data);        /* compute A,B, and P */
      }
 		// Go back to beginning of float array

   float *fptr = abpdata->data;
   short *sptr = (short *)abpdata->data;

  // Convert the floating point values to shorts
  for(i=0; i<abpdata->info.gates; i++)
  {
			
	float a=*fptr++,b=*fptr++,p=*fptr++;
	if(a != 0.0 || b != 0.0)      
		{      
		*sptr++ = hscale * log(a*a+b*b) + offset;
		*sptr++ = atan2(b,a) * velfactor;
		}
	else      
		{      
		*sptr++ = 0;
		*sptr++ = 0;
		}
	*sptr++ =  16832;//scale * log(p) + offset;

	// Do V's
	a=*fptr++,b=*fptr++,p=*fptr++;
	if(a != 0.0 || b != 0.0)      
		{      
		*sptr++ = hscale * log(a*a+b*b) + offset;
		*sptr++ = atan2(b,a) * velfactor;
		}
	else      
		{      
		*sptr++ = 0;
		*sptr++ = 0;
		}
	*sptr++ =  16832;//scale * log(p) + offset;
			
	// Cross Correlations
	a=*fptr++,b=*fptr++;
	if(a != 0.0 || b != 0.0)      
		{      
		*sptr++ = hscale * log(a*a+b*b) + offset;
		*sptr++ = atan2(b,a) * velfactor;
		}
	else      
		{      
		*sptr++ = 0;
		*sptr++ = 0;
		}

	  }
  }


/* HV pulse pair algorithm */
/* format is ABPABPAB */
/* where ABP is for H */
/* then  ABP is for V */
/* then  AB is the cross term for HV */
/*                              */
/* The format is now: (H)IQ(V)IQ */
void hvpulsep(int gates,short *iqptr,short *lagbuf,float *abp)
   {
   int           i;
   double        HIold,HQold,HI,HQ,VIold,VQold,VI,VQ;
   float *abp_orig = abp;
   
   for(i=0; i<gates; i++)
      {
      /* read in an IH */
      HIold = *lagbuf++;
      HI = *iqptr++;
       
      /* read in a QH */
      HQold = *lagbuf++;
      HQ = *iqptr++;

      /* read in an IV */
      VIold = *lagbuf++;
      VI = *iqptr++;
       
      /* read in a QV */
      VQold = *lagbuf++;
      VQ = *iqptr++;

      /* lag zero and one autocorrelation H */
      *abp++ +=  VIold * VI   + VQold * VQ;        /* do ah */
      *abp++ +=  VIold * VQ   - VQold * VI;        /* do bh */
      *abp++ +=  VI    * VI   + VQ    * VQ;        /* do ph */

      /* lag zero and one autocorrelation V */
      *abp++ +=  HIold * HI   + HQold * HQ;        /* do av */
      *abp++ +=  HIold * HQ   - HQold * HI;        /* do bv */
      *abp++ +=  HI    * HI   + HQ    * HQ;        /* do pv */

      *abp++ += VI * HI + VQ * HQ;      /* real part of cross term */
      *abp++ += VQ * HI - VI * HQ;      /* imaginary part of cross term */
      }
   }   


/* Stokes parameter algorithm added 10/14/01 GRG */
/* Output format is I1, I2, U, V */
/* as described in the COBRA document */
/* in the Modified Stokes Parameter section */
/*												*/
/* The format is now: (H)IQ(V)IQ */
/* In the descriptions:		*/
/* Xh = HI + jHQ, Xv = VI + jVQ, and ' indicates complex conjugation */
/* e.g. Xh' = HI - jHQ.
/* X represents any one of N(oise), T(ransmitter), or R(ange gate) computations */
/* as described in the COBRA document */

void stokes(int indx, int gates, short *iqptr, float Knorm, float *abp)
   {
   int           i;
   double        HI,HQ,VI,VQ;
   float *abp_orig = abp;
   
   for(i=0; i<gates; i++)
      {
      /* read in an HI */
      HI = *iqptr++;
       
      /* read in a HQ */
      HQ = *iqptr++;

      /* read in an VI */
      VI = *iqptr++;
       
      /* read in a VQ */
      VQ = *iqptr++;
	if(indx)
		{
		*abp++ +=	HI*HI + HQ*HQ;			/* Do I1 = sum(Xh'*Xh) */
		*abp++ +=	VI*VI + VQ*VQ;			/* Do I2 = sum(Xv'*Xv) */
		*abp++ +=	2.0*(HI*VI + HQ*VQ);	/* Do 2Re(sum(Xh'*Xv)  */
		*abp++ +=	2.0*(HQ*VI - VQ*HI);	/* Do 2Im(sum(Xh'*Xv)  */
		}
	else
		{
		*abp++ =	HI*HI + HQ*HQ;			/* Do I1 = sum(Xh'*Xh) */
		*abp++ =	VI*VI + VQ*VQ;			/* Do I2 = sum(Xv'*Xv) */
		*abp++ =	2.0*(HI*VI + HQ*VQ);	/* Do 2Re(sum(Xh'*Xv)  */
		*abp++ =	2.0*(HQ*VI - VQ*HI);	/* Do 2Im(sum(Xh'*Xv)  */
		}
      }
   }   

/* FFTMOMENTS computes the 0, 1, and 2th moments in one gate */

void fftmoments(complex gt[], DATABLOCK *abpdata, float *out)
	{
	unsigned int i;
	float Mom0 = 0, Mom1 = 0, Mom2 = 0, varg, Norm, findx;
	complex *ct = gt;

	float scale =  10.0 / log(10.0) / .004;  /* approx = 1085.7 */
    float hscale = 0.5 * scale;
    float offset = - 2.0 * scale * log(32768.0 / (double)0x10000)
	   - scale * log((double)abpdata->info.hits);
    float velfactor = 65536.0 / TWOPI;

	Norm = (float)abpdata->info.hits;

	/* Get the spectrum into array gt */
	fft(gt, abpdata->info.hits);

	/* Now computed the first three moments */
	/* 0th Moment, or power */
	for(i = 0; i < abpdata->info.hits; i++)
		{
		Mom0 += sqrt(gt[i].x*gt[i].x + gt[i].y*gt[i].y)- FFT_NOISE;	/* Summing up the power terms (Mom0) */ 
		}
	Mom0 /= Norm;

	/* 1st Moment, or mean velocity */
	for(i = 0; i < abpdata->info.hits; i++)
		{
		Mom1 += (float)i*(sqrt(gt[i].x*gt[i].x + gt[i].y*gt[i].y));
		}
	Mom1 /= Norm*Norm*Mom0; /* Should give mean velocity in terms of index, i */

	/* 2nd Moment, or spectral width */
	for(i = 0; i < abpdata->info.hits; i++)
		{
		findx = (float)i;
		Mom2 += (findx-Mom1)*(findx-Mom1)*(sqrt(gt[i].x*gt[i].x + gt[i].y*gt[i].y));
		}
	Mom2 /= Norm*Norm*Mom0; /* Should give spectral width in terms of indices */
	
	/* Write to the output buffer using simple16 format */
	
	varg = 3.14159*Mom1;			/* Velocity full scale is varg */

	short *sptr = (short *)abpdata->data;

	// convert to 16 bit integer representation: DATA_SIMPLE16, almost.  
	for(i=0; i<abpdata->info.gates; i++)
		{   
		if(Mom2 !=0)      
			{      
			*sptr++ = hscale * log(Mom2) + offset;
			}
		else      
			{      
			*sptr++ = 0;
			}
		*sptr++ = varg * velfactor;	
		*sptr++ =  scale * log(Mom0) + offset;
		}
	abpdata->info.dataformat = DATA_SIMPLE16;
	return;
	}