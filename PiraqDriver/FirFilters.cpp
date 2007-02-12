/////////////////////////////////////////////////////////////////////////////////////////
// FILE NAME: FirFilter.cpp
//
// DESCRIPTION: FIRFILTER Class implementation. This class handles the Fir Filters
//
/////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "FirFilters.h"
#include <math.h>



/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: FIRFILTER::FIRFILTER( unsigned int *pPiraq_BaseAddress)
//
// DESCRIPTION: Constructor - Sets the address for the two Fir Filter Chips 
//
// INPUT PARAMETER(s): 
// unsigned int *pPiraq_BaseAddress - Base address of Piraq card
//
// OUTPUT PARAMETER(s): None
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
FIRFILTER::FIRFILTER( unsigned int *pPiraq_BaseAddress)
{
	m_pPiraq_BaseAddress	=  (unsigned int*)pPiraq_BaseAddress;

	FIRCH1_I         = (unsigned short *) (((unsigned char *)m_pPiraq_BaseAddress) + 0);
	FIRCH1_Q	     = (unsigned short *) (((unsigned char *)m_pPiraq_BaseAddress) + 0x400);
	FIRCH2_I		 = (unsigned short *) (((unsigned char *)m_pPiraq_BaseAddress) + 0x800);
	FIRCH2_Q         = (unsigned short *) (((unsigned char *)m_pPiraq_BaseAddress) + 0xC00);

}




/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: FIRFILTER::~FIRFILTER()
//
// DESCRIPTION: Destructor 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): None
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
FIRFILTER::~FIRFILTER()
{
}






/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: FIRFILTER::Gaussian(float fFrequencyHz, long lTransmitPulseWidthNs,  long lNumCodeBits)
//
// DESCRIPTION: Sets the Fir Filters to use a Gaussian curve 
//
// INPUT PARAMETER(s): 
// float fFrequencyHz - Piraq Clock Frequency in Hertz
// long lTransmitPulseWidthNs - Transmit pulse width in nanoseconds
// long lNumCodeBits - Used to change the bandwidth when using coding
//
// OUTPUT PARAMETER(s): None
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
void FIRFILTER::Gaussian(float fFrequencyHz, long lTransmitPulseWidthNs, long lNumCodeBits)
{

	// Calculate the required terms for the FIR initialization using a gausian gate filter.
	double dTimeBandwidthProduct = 1.04; // From Doviac & Zrnic
	double dTimeStep		= 1 / fFrequencyHz; // ns or 1/Hz
	double dBandWidth		= dTimeBandwidthProduct / (2 * lTransmitPulseWidthNs * 1e-9); // Hz
	double dSigma				= ( sqrt((2 * log( 2.0 )))  / (2.0 * M_PI * dBandWidth) ); // Standard Deviation in Hz

	// FIR chip initialization
	ClearFilter();				                            

	// Sum
	m_lCoeffSum = 0;
	
	// Depending on the gate length, the FIR is either in /1 , /2 or /4 mode      
  if ( lTransmitPulseWidthNs < 1000 /*ns*/ )    
  { 
		// Full Rate. Number of taps = 32 (symmetric) 

		// Determine which set of control register values to select
		m_iFirRegisterGroup = 0;

		for(int i=0; i<32; i++)	      
		{
			// Calculate Coefficient

			short shValue = (short)((SCALE * exp( -pow(((31-i)*dTimeStep),2.0) / (2*2*dSigma*dSigma) )) + .5);
			
			// Both the A and B paths get same coefficients */
			// Both channel 1 and channel 2 get same settings */
			
			if(i&1)		             
			{
				// If this gate is an odd tag number

				// Channel A Chip 1
				FIRCH1_Q[128 + (2 * i) - 1] = shValue;
				// Channel B Chip 1
				FIRCH1_Q[192 + (2 * i) - 1] = shValue;

				// Channel A Chip 2
				FIRCH2_Q[128 + (2 * i) - 1] = shValue;
				// Channel B Chip 2
				FIRCH2_Q[192 + (2 * i) - 1] = shValue;
			}
			else			 
			{
				// If this gate is an even tag number

				// Channel A Chip 3
				FIRCH1_I[128 + (2 * i) + 1] = shValue;
				// Channel B Chip 3
				FIRCH1_I[192 + (2 * i) + 1] = shValue;

				// Channel A Chip 4
				FIRCH2_I[128 + (2 * i) + 1] = shValue;
				// Channel B Chip 4
				FIRCH2_I[192 + (2 * i) + 1] = shValue;

				m_lCoeffSum = m_lCoeffSum + shValue;
			}
		}

		m_lCoeffSum = m_lCoeffSum * 2;

	}
	else   if ( lTransmitPulseWidthNs < 2000 /*ns*/ )    
  
	{
		// Half Rate Mode - Number of taps = 64 (symmetric)

		// Determine which set of control register values to select
		m_iFirRegisterGroup = 2;

		for(int i=0; i<64; i++)	
		{
			// Calculate Coefficient
			short shValue = (short)((SCALE * exp( -pow(((63-i)*dTimeStep),2.0) / (2*2*dSigma*dSigma) )) + .5);

			switch(i%4)
			{
				case 0: 
					FIRCH1_I[128 + i + 0] = shValue;
					FIRCH1_I[192 + i + 0] = shValue;
					FIRCH2_I[128 + i + 0] = shValue;
					FIRCH2_I[192 + i + 0] = shValue;
					m_lCoeffSum = m_lCoeffSum + shValue;
					break;

				case 1: 
					FIRCH1_Q[128 + i - 1] = shValue;
					FIRCH1_Q[192 + i - 1] = shValue;
					FIRCH2_Q[128 + i - 1] = shValue;
					FIRCH2_Q[192 + i - 1] = shValue;
					break;

				case 2: 
					FIRCH1_I[128 + i - 1] = shValue;
					FIRCH1_I[192 + i - 1] = shValue;
					FIRCH2_I[128 + i - 1] = shValue;
					FIRCH2_I[192 + i - 1] = shValue;
					m_lCoeffSum = m_lCoeffSum + shValue;
					break;

				case 3: 
					FIRCH1_Q[128 + i - 2] = shValue;
					FIRCH1_Q[192 + i - 2] = shValue;
					FIRCH2_Q[128 + i - 2] = shValue;
					FIRCH2_Q[192 + i - 2] = shValue;
					break;
			}
		}

		m_lCoeffSum = m_lCoeffSum * 2;

	}
	else				
	{
		// Determine which set of control register values to select
		m_iFirRegisterGroup = 4;

		// Quarter Rate. Number of taps = 128 (symmetric) 
		for(int i=0; i<128; i++)
		{
			// Calculate Coefficient
			short shValue = (short)((SCALE * exp( -pow(((127-i)*dTimeStep),2.0) / (2*2*dSigma*dSigma) )) + .5);

			if(i&1)		
			{
				// If this gate is an odd tag number
				FIRCH1_Q[128 + ((i-1)/2)] = shValue;
				FIRCH1_Q[192 + ((i-1)/2)] = shValue;
				FIRCH2_Q[128 + ((i-1)/2)] = shValue;
				FIRCH2_Q[192 + ((i-1)/2)] = shValue;
			}
			else		
			{
				// If this gate is an even tag number
				FIRCH1_I[128 + (i/2)]   = shValue;
				FIRCH1_I[192 + (i/2)]   = shValue;
				FIRCH2_I[128 + (i/2)]   = shValue;
				FIRCH2_I[192 + (i/2)]   = shValue;
				m_lCoeffSum = m_lCoeffSum + shValue;
			}
		}

		m_lCoeffSum = m_lCoeffSum * 2;

	}
	
	// Start up the filter
	StartFilter();
}





/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: FIRFILTER::FIR_HalfBandWidth(float fFrequencyHz, long lTransmitPulseWidthNs,   long lNumCodeBits)
//
// DESCRIPTION: Sets the Fir Filters to use a half band Width Filter
//
// INPUT PARAMETER(s): 
// long lTransmitPulseWidthNs - Transmit pulse in nanoseconds
// float fFrequencyHz - Radar Frequency in Hertz
// long lNumCodeBits - Used to change the bandwidth when using coding
//
// OUTPUT PARAMETER(s): None
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
void FIRFILTER::FIR_HalfBandWidth(float fFrequencyHz, long lTransmitPulseWidthNs, long lNumCodeBits)
{
	// FIR chip initialization
	// Depending on the gate length, the FIR is either in /1 , /2 or /4 mode 

	// Sum
	m_lCoeffSum = 0;

	if ( lTransmitPulseWidthNs < 1000 /*ns*/ )    		
	{
		// Full Rate Mode
		double dWC = 2.0 * M_PI * fFrequencyHz / (1 * 4.0);

		// Determine which set of control register values to select
		m_iFirRegisterGroup = 0;

		// "64" Mhz 
		double dTimeStep = 1.0 / fFrequencyHz;	

		// Full Rate. Number of taps = 64 (symmetric)
		for(int i=0; i<32; i++)	 
		{
			// Calcuate Coefficients
			short shValue = (short)(SCALE * Sinc((31-i) * dTimeStep * dWC));

			if(i&1)	// if odd 
			{
				// Channel 1
				FIRCH1_Q[128 + (2 * i) - 1] = shValue;
				FIRCH1_Q[192 + (2 * i) - 1] = shValue;

				// Channel 2
				FIRCH2_Q[128 + (2 * i) - 1] = shValue;
				FIRCH2_Q[192 + (2 * i) - 1] = shValue;
			}
			else	// if even 
			{
				// Channel 1
				FIRCH1_I[128 + (2 * i) + 1] = shValue;
				FIRCH1_I[192 + (2 * i) + 1] = shValue;

				// Channel 2
				FIRCH2_I[128 + (2 * i) + 1] = shValue;
				FIRCH2_I[192 + (2 * i) + 1] = shValue;
				m_lCoeffSum = m_lCoeffSum + shValue;
			}
		}

		m_lCoeffSum = m_lCoeffSum * 2;

	}
	else 	if ( lTransmitPulseWidthNs < 2000 /*ns*/ )    		 
	{
		// half rate mode

		// Determine which set of control register values to select
		m_iFirRegisterGroup = 2;
		
		double dWC = 2.0 * M_PI * fFrequencyHz / (2 * 4.0);

		// "32" MHz
		double dTimeStep = 2.0 / fFrequencyHz;
    
		// Half Rate. Number of taps = 64 (symmetric) 
		for(int i=0; i<64; i++)	
		{
			// Calcuate Coefficients
			short shValue = (short)(SCALE * Sinc((63-i) * dTimeStep * dWC));

			switch(i%4)
			{
				case 0:
					FIRCH1_I[128 + i + 0] = shValue;
					FIRCH1_I[192 + i + 0] = shValue;
					m_lCoeffSum = m_lCoeffSum + shValue;
				break;

				case 1:
					FIRCH1_Q[128 + i - 1] = shValue;
					FIRCH1_Q[192 + i - 1] = shValue;
				break;

				case 2:
					FIRCH1_I[128 + i - 1] = shValue;
					FIRCH1_I[192 + i - 1] = shValue;
					m_lCoeffSum = m_lCoeffSum + shValue;
				break;

				case 3:
					FIRCH1_Q[128 + i - 2] = shValue;
					FIRCH1_Q[192 + i - 2] = shValue;
				break;
			}
		}

		m_lCoeffSum = m_lCoeffSum * 2;

	}
	else										 
	{
		// Quarter Rate Mode

		// Determine which set of control register values to select 
		m_iFirRegisterGroup = 4;
		
		// "16" Mhz
		double dTimeStep = 4.0 / fFrequencyHz;  
		
		double dWC = 2.0 * M_PI * fFrequencyHz / (4 * 4.0);

		// Quarter Rate. Number of taps = 128 (symmetric)
		for(int i=0; i<128; i++)	 
		{
			// Calcuate Coefficients
			short shValue = (short)(SCALE * Sinc((127-i) * dTimeStep * dWC));

			if(i&1)	 
			{
				// If odd
				FIRCH1_Q[128 + (i - 1) / 2] = shValue;
				FIRCH1_Q[192 + (i - 1) / 2] = shValue;
			}
		  else	
			{
				// If even 
				FIRCH1_I[128 + i / 2] = shValue;
				FIRCH1_I[192 + i / 2] = shValue;
				m_lCoeffSum = m_lCoeffSum + shValue;

			}
		}

		m_lCoeffSum = m_lCoeffSum * 2;

	}

	StartFilter();

}   



/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: FIRFILTER::FIR_BoxCar(float fFrequencyHz, long lTransmitPulseWidthNs,  long lNumCodeBits)
//
// DESCRIPTION:  Sets the Fir Filters to use a Box Car curve
//
// INPUT PARAMETER(s): 
// long lTransmitPulseWidthNs - Transmit pulse in nanoseconds
// float fFrequencyHz - Radar Frequency in Hertz
// long lNumCodeBits - Used to change the bandwidth when using coding
//
// OUTPUT PARAMETER(s): None
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
void FIRFILTER::FIR_BoxCar(float fFrequencyHz, long lTransmitPulseWidthNs,  long lNumCodeBits)
{
	// FIR chip initialization
	// Depending on the gate length, the FIR is either in /1 , /2 or /4 mode 

	// Sum
	m_lCoeffSum = 0;

	if ( lTransmitPulseWidthNs < 1000 /*ns*/ )    		
	{
		// Full Rate Mode
		int iTransmitPulseWidthClockCylces = (int)ceil((lTransmitPulseWidthNs*fFrequencyHz*1e-9*1)/2);

		// Determine which set of control register values to select
		m_iFirRegisterGroup = 0;

		// Full Rate. Number of taps = 64 (symmetric)
		for(int i=0; i<32; i++)	 
		{
			short shValue;

			// Calcuate Coefficients
			if(i < iTransmitPulseWidthClockCylces)
			{
				shValue = (short)SCALE;
			}
			else
			{
				shValue = 0;
			}

			if(i&1)	// if odd 
			{
				// Channel 1
				FIRCH1_Q[128 + (2 * i) - 1] = shValue;
				FIRCH1_Q[192 + (2 * i) - 1] = shValue;

				// Channel 2
				FIRCH2_Q[128 + (2 * i) - 1] = shValue;
				FIRCH2_Q[192 + (2 * i) - 1] = shValue;
			}
			else	// if even 
			{
				// Channel 1
				FIRCH1_I[128 + (2 * i) + 1] = shValue;
				FIRCH1_I[192 + (2 * i) + 1] = shValue;

				// Channel 2
				FIRCH2_I[128 + (2 * i) + 1] = shValue;
				FIRCH2_I[192 + (2 * i) + 1] = shValue;
				m_lCoeffSum = m_lCoeffSum + shValue;
			}
		}

		m_lCoeffSum = m_lCoeffSum * 2;
	
	}
	else 	if ( lTransmitPulseWidthNs < 2000 /*ns*/ )    		 
	{
		// half rate mode
		int iTransmitPulseWidthClockCylces = (int)ceil((lTransmitPulseWidthNs*fFrequencyHz*1e-9*2)/2);

		// Determine which set of control register values to select
		m_iFirRegisterGroup = 2;
		
    
		// Half Rate. Number of taps = 64 (symmetric) 
		for(int i=0; i<64; i++)	
		{
			// Calcuate Coefficients
			short shValue;
			if(i < iTransmitPulseWidthClockCylces)
			{
				shValue = (short)SCALE;
			}
			else
			{
				shValue = 0;
			}

			switch(i%4)
			{
				case 0:
					FIRCH1_I[128 + i + 0] = shValue;
					FIRCH1_I[192 + i + 0] = shValue;
					m_lCoeffSum = m_lCoeffSum + shValue;
				break;

				case 1:
					FIRCH1_Q[128 + i - 1] = shValue;
					FIRCH1_Q[192 + i - 1] = shValue;
				break;

				case 2:
					FIRCH1_I[128 + i - 1] = shValue;
					FIRCH1_I[192 + i - 1] = shValue;
					m_lCoeffSum = m_lCoeffSum + shValue;
				break;

				case 3:
					FIRCH1_Q[128 + i - 2] = shValue;
					FIRCH1_Q[192 + i - 2] = shValue;
				break;
			}
		}
				
		m_lCoeffSum = m_lCoeffSum * 2;

	}
	else										 
	{
		// Quarter Rate Mode
		int iTransmitPulseWidthClockCylces = (int)ceil((lTransmitPulseWidthNs*fFrequencyHz*1e-9*4)/2);

		// Determine which set of control register values to select 
		m_iFirRegisterGroup = 4;
				
		// Quarter Rate. Number of taps = 128 (symmetric)
		for(int i=0; i<128; i++)	 
		{
			// Calcuate Coefficients
			short shValue;
			if(i < iTransmitPulseWidthClockCylces)
			{
				shValue = (short)SCALE;
			}
			else
			{
				shValue = 0;
			}

			if(i&1)	 
			{
				// If odd
				FIRCH1_Q[128 + (i - 1) / 2] = shValue;
				FIRCH1_Q[192 + (i - 1) / 2] = shValue;
			}
		  else	
			{
				// If even 
				FIRCH1_I[128 + i / 2] = shValue;
				FIRCH1_I[192 + i / 2] = shValue;
				m_lCoeffSum = m_lCoeffSum + shValue;
			}
		}

		m_lCoeffSum = m_lCoeffSum * 2;
	}

	StartFilter();

}   







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: FIRFILTER::StartFilter()
//
// DESCRIPTION: Starts the filter by writing the control group 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): None
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
void FIRFILTER::StartFilter()
{
// FINISH Make sure the filters are set up

	unsigned short FIR_cfg[][13] = 
	{
		
		//Group 1:
		0x24D8,0x6108,0x04DC,0x6108,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, // Full rate odd
		0x24D8,0x6181,0x04DC,0x6181,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, // Full rate even

		//Group 2:
		0x248B,0x2E12,0x048F,0x2E12,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, // Half rate odd
		0x248B,0x2E91,0x048F,0x2E91,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, // Half rate even

		//Group 3:
		0x2402,0x0214,0x0406,0x0214,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, // Quarter rate odd
		0x2402,0x0292,0x0406,0x0292,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000  // Quarter rate even

	};
	
	// Load the correct group of FIR_cfg[][13] 

	for(int i=0; i<13; i++)
	{
		*(FIRCH1_I + i)  = FIR_cfg[m_iFirRegisterGroup  ][i];	// odd 
		*(FIRCH1_Q + i)  = FIR_cfg[m_iFirRegisterGroup+1][i];	// even 
		*(FIRCH2_I + i)  = FIR_cfg[m_iFirRegisterGroup  ][i];	// odd 
		*(FIRCH2_Q + i)  = FIR_cfg[m_iFirRegisterGroup+1][i];	// even 
	}

	// GAIN_REG:	0x00f0	0000 0000 1111 0000b
	//		The chip's output gain is set using F an S according to the following formula:
	//						(S-20)
	//				GAIN = 2  (1+F/16)(DC_GAIN)
	//	where:
	//			DC_GAIN is the sum of the fillter coefficients.  Unity gain, according to this
	//			formula, will map the MSB of the 12 bit input data (A|11 or B|11) into the MSB 
	//			of the selected output word.

	// figure out the gain based on the sum of the coefficients 
	// If 2^11 is input to the AI channel, it gets multiplied by 'sum' into a 32 bit accumulator 
	// Then you want to multiply it by the gain so that it equals 2^15 and lines up with AO15 
	// The A015 line is really the 2^31 line of the accumulator, so 
	// In other words 2^31 = sum * 2^11 * GAIN   ->   GAIN = 2^31 / (sum * 2^11)  
	// Or further simplifying, GAIN = 2^20 / sum  
	// GAIN is set with 2^s * (1 + F/16) 
	// 2^s * (1 + F/16) = 2^20 / sum      ->     2^(s-20) * (1 + F/16) = 1 / sum  
	// 2^(20-s)/(1 + F/16) = sum 

   int s = (int)(20.0 - log((double)m_lCoeffSum) / log(2.0));
   int f = (int)(16.0 * (pow(2.0,20.0-s)/(double)m_lCoeffSum - 1.0) + 0.5);

   FIRCH1_I[6]  = 0x2000 + (16 * s) + f;
   FIRCH1_Q[6]  = 0x2000 + (16 * s) + f;
   FIRCH2_I[6]  = 0x2000 + (16 * s) + f;
   FIRCH2_Q[6]  = 0x2000 + (16 * s) + f;

}








/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: FIRFILTER::ClearFilter()
//
// DESCRIPTION: Writes zeros to all filter locations 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): None
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
void FIRFILTER::ClearFilter()
{
	for(int i=128; i<256; i++)
	{
		FIRCH1_I[i] = FIRCH1_Q[i] = 0;
	}
}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: FIRFILTER::Sinc(double arg)
//
// DESCRIPTION: NOT TESTED 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): None
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
double	FIRFILTER::Sinc(double arg)
{
	if(arg == 0.0)
	{
		return(1.0);
	}

	return(sin(arg) / arg);
}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: FIRFILTER::ReturnFilterValues(unsigned short	*pFIR, unsigned int *piCount )
//
// DESCRIPTION: Returns the last values used to set up the Fir Filters 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): 
// unsigned short	*pFIR - An array to store the values in
// unsigned int *piCount - The number of values
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
void FIRFILTER::ReturnFilterValues(unsigned short	*pFIR, unsigned int *piCount )
{
	* piCount = (m_iFirRegisterGroup+1)*32;

	for(unsigned int i=0; i<((m_iFirRegisterGroup+1)*32); i++)
	{
		if(i&1)		             
		{
			// Channel A Chip 1
			pFIR[i] = FIRCH1_Q[128 + (2 * i) - 1];
		}
		else			 
		{
			// Channel A Chip 3
			pFIR[i] = FIRCH1_I[128 + (2 * i) + 1];
		}
	}
}



/////////////////////////////////////////////////////////////////////////////////////////
/* Used for documentation

unsigned short	FIR_cfg[][13] = {

// The following initialization streams are used to configure the fir GC2011A Digital Filter
// Group 1 is used to initialize when there are less than 32 tagged gate pulses
// Group 2 is used to initialize when there are less than 64 tagged gate pulses
// Group 3 is used to initialize when there are less than 128 tagged gate pulses
// each 16 bit control register is named as follows:
//	0 ) APATH_REG0		see data sheet page 29
//  1 ) APATH_REG1		see data sheet page 31
//  2 ) BPATH_REG0		see data sheet page 29
//  3 ) BPATH_REG1		see data sheet page 31
//  4 ) CASCADE_REG		see data sheet page 32
//  5 ) COUNTER_REG		see data sheet page 32
//  6 ) GAIN_REG		see data sheet page 32
//  7 ) OUTPUT_REG		see data sheet page 34
//  8 ) SNAP_REGA		see data sheet page 35
//  9 ) SNAP_REGB		see data sheet page 35
//  10) SNAP_START_REG	see data sheet page 36
//  11) ONE_SHOT		see data sheet page 36
//  12) NEW_MODES		see data sheet page 37

// In the documentation this setup is described as a Real to complex Quadrature Down Conversion
//    See GC2011A 3.3v Digital Filter Chip Data Sheet Rev 1.1 Page 20
//

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Group 1:
//	documenting Full Rate odd symmetry
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 0x24D8,0x6108,0x04DC,0x6108,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000,
/// full rate odd 

// APATH_REG0:	0x24d8 0010 0100 1101 1000b
0x24D8,
//	field name			bits		value and meaning
//		ACCUM			0,1			00		- don't accumulate (full rate output)
//		UNSIGNED		2			0		- filter cell adder is in signed mode
//		COEF_SEL		3-7			11011	- 0x1b use coefficient register 1
//		RATE			8,9			00		- 0x00 Full rate input
//		NEG_IN			10-12		100		- 0x04 always negate sample
//		AB_SEL			13			1		- Select input A
//		DELAY_SEL		14,15		00		- no delay

// APATH_REG1:	0x6108	0110 0001 0000 1000b
0x6108,
//	field name		bits		value and meaning
//		FEED_BACK		0-4			01000	- Full rate even symmetry and A-path cascade mode
//		ANTI_SYM		5			0		- do not complement the input data			//????????
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			0		- clear for even symmetry filter
//		REV_DELAY		8-12		00001	- full rate filters
//		FOR_DELAY		13-15		011		- quarter rate filters

// BPATH_REG0:	0x04dc 0000 0100 1101 1100b
0x04DC,
//	field name			bits		value and meaning
//		ACCUM			0,1			00		- don't accumulate (full rate output)
//		UNSIGNED		2			1		- filter cell adder is in unsigned mode
//		COEF_SEL		3-7			11011	- use coefficient register 1
//		RATE			8,9			00		- quarter rate input
//		NEG_IN			10-12		001		- negate even time full rate samples
//		AB_SEL			13			0		- Select input B
//		DELAY_SEL		14,15		00		- set to no delay

// BPATH_REG1:	0x6108	0110 0001 0000 1000b
0x6108,
//	field name			bits		value and meaning
//		FEED_BACK		0-4			01000	- Full rate even symmetry and A-path cascade mode
//		ANTI_SYM		5			0		- do not complement the input data			//????????
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			0		- clear for even symmetry filter
//		REV_DELAY		8-12		00001	- full rate filters
//		FOR_DELAY		13-15		011		- quarter rate filters

// CASCADE_REG:	0x2000	0010 0000 0000 0000b
0x2000,
//	field name			bits		value and meaning
//		SYNC_COEF		0			0		- Do not synchronize coefficient data with system clock.
//		CASCADE			9-15		1		- //??????????????

// COUNT_reg:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//		CNT				0-15		0		- Preset counter to (16*cnt+15)

// GAIN_REG:	0x00f0	0000 0000 1111 0000b
//		The chip's output gain is set using F an S according to the following formula:
//						(S-20)
//				GAIN = 2  (1+F/16)(DC_GAIN)
//	where:
//			DC_GAIN is the sum of the fillter coefficients.  Unity gain, according to this
//			formula, will map the MSB of the 12 bit input data (A|11 or B|11) into the MSB 
//			of the selected output word.
0x00F0,
//	field name			bits		value and meaning
//	F					0-3			0000	- the 4 bit gain fraction
//	S					4-7			1111	- the 4 bit gain exponent
//	ROUND				8-14	0000 000	- truncate

// OUTPUT_REG:	0x004c	0000 0000 0100 1100b
0x004C,
//	field name			bits		value and meaning
//	NEG_OUT				0,1			00		- Don't Negate
//	24BIT_MODE			2			1		- enable 24 bit mode
//	24BIT_OUT			3			1		- enable 24 bit output mode
//	MUX_MODE			4,5			00		- mux mode is off
//	PATH_ADD			6			1		- adds A-path and B-path results
//	DAV_POLARITY		7			0		- do not invert polarity of data valid (DAV) strobe

//	SNAP_REGA:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_REGB:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_START_REG:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//	TRIGGER				0,1			0		- immediately
//	READ_MODE			2,3			0		- read words
//	ARMED				4			0		- Arm snapshot memory
//	A_DONE				5			0		- Bit goes high when the A-Half snapshot is done
//	B_DONE				6			0		- Bit goes high when the B-Half snapshot is done
//	SYNC_OUT			8,9			00		- Sync_out is SI* delayed by 4 clocks (SYNC_OFF=0)
//	SYNC_OFF			10			0		- Turn sync on.

//	ONE_SHOT:	0x0000	0000 0000 0000 0000b
//			value stored here is unused. and register is write only
0x0000,

//	NEW_MODES:	0x0000	0000 0000 0000 0000b
//	field name			bits		value and meaning
//	REVISION			0-7			xxx		- These bits read back the current mask revision number
//	POWER_DOWN			8			0		- disable static power down mode.
//	DISABLE_CLOCK_LOSS_DETECT
//						9			0		- This bit should be kept low
//	POWER_DOWN_STATUS	10,11		00		- return power down status as low going strobe.
//	INV_MSB_AOUT		12			0		- do not invert MSB of A-output
//	INV_MSB_BOUT		13			0		- do not invert MSB of B-output
//	INV_MSB_AIN			14			0		- do not invert MSB of A-input
//	INV_MSB_AIN			15			0		- do not invert MSB of B-input

0x0000,  // full rate odd symmetry    

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Group 1:
//	documenting Full Rate even symmetry
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   0x24D8,0x6181,0x04DC,0x6181,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000,
// full rate even 

// APATH_REG0:	0x24d8 0010 0100 1101 1000b
0x24D8,
//	field name			bits		value and meaning
//		ACCUM			0,1			00		- don't accumulate (full rate output)
//		UNSIGNED		2			0		- filter cell adder is in signed mode
//		COEF_SEL		3-7			11011	- 0x1b use coefficient register 1
//		RATE			8,9			00		- 0x00 Full rate input
//		NEG_IN			10-12		100		- 0x04 always negate sample
//		AB_SEL			13			1		- Select input A
//		DELAY_SEL		14,15		00		- no delay

// APATH_REG1:	0x6181	0110 0001 1000 0001b
0x6181,
//	field name		bits		value and meaning
//		FEED_BACK		0-4			00001	- Full rate odd symmetry
//		ANTI_SYM		5			0		- do not complement the input data			//????????
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			1		- odd symmetry
//		REV_DELAY		8-12		00001	- full rate filters
//		FOR_DELAY		13-15		011		- quarter rate filters

// BPATH_REG0:	0x04dc 0000 0100 1101 1100b
0x04DC,
//	field name			bits		value and meaning
//		ACCUM			0,1			00		- don't accumulate (full rate output)
//		UNSIGNED		2			1		- filter cell adder is in unsigned mode
//		COEF_SEL		3-7			11011	- use coefficient register 1
//		RATE			8,9			00		- quarter rate input
//		NEG_IN			10-12		001		- negate even time full rate samples
//		AB_SEL			13			0		- Select input B
//		DELAY_SEL		14,15		00		- set to no delay

// BPATH_REG1:	0x6181	0110 0001 1000 0001b
0x6181,
//	field name		bits		value and meaning
//		FEED_BACK		0-4			00001	- Full rate odd symmetry
//		ANTI_SYM		5			0		- do not complement the input data			//????????
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			1		- odd symmetry
//		REV_DELAY		8-12		00001	- full rate filters
//		FOR_DELAY		13-15		011		- quarter rate filters

// CASCADE_REG:	0x2000	0010 0000 0000 0000b
0x2000,
//	field name			bits		value and meaning
//		SYNC_COEF		0			0		- Do not synchronize coefficient data with system clock.
//		CASCADE			9-15		1		- //??????????????

// COUNT_reg:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//		CNT				0-15		0		- Preset counter to (16*cnt+15)

// GAIN_REG:	0x00f0	0000 0000 1111 0000b
//		The chip's output gain is set using F an S according to the following formula:
//						(S-20)
//				GAIN = 2  (1+F/16)(DC_GAIN)
//	where:
//			DC_GAIN is the sum of the fillter coefficients.  Unity gain, according to this
//			formula, will map the MSB of the 12 bit input data (A|11 or B|11) into the MSB 
//			of the selected output word.
0x00F0,
//	field name			bits		value and meaning
//	F					0-3			0000	- the 4 bit gain fraction
//	S					4-7			1111	- the 4 bit gain exponent
//	ROUND				8-14	0000 000	- truncate

// OUTPUT_REG:	0x004c	0000 0000 0100 1100b
0x004C,
//	field name			bits		value and meaning
//	NEG_OUT				0,1			00		- Don't Negate
//	24BIT_MODE			2			1		- enable 24 bit mode
//	24BIT_OUT			3			1		- enable 24 bit output mode
//	MUX_MODE			4,5			00		- mux mode is off
//	PATH_ADD			6			1		- adds A-path and B-path results
//	DAV_POLARITY		7			0		- do not invert polarity of data valid (DAV) strobe

//	SNAP_REGA:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_REGB:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_START_REG:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//	TRIGGER				0,1			0		- immediately
//	READ_MODE			2,3			0		- read words
//	ARMED				4			0		- Arm snapshot memory
//	A_DONE				5			0		- Bit goes high when the A-Half snapshot is done
//	B_DONE				6			0		- Bit goes high when the B-Half snapshot is done
//	SYNC_OUT			8,9			00		- Sync_out is SI* delayed by 4 clocks (SYNC_OFF=0)
//	SYNC_OFF			10			0		- Turn sync on.

//	ONE_SHOT:	0x0000	0000 0000 0000 0000b
//			value stored here is unused. and register is write only
0x0000,

//	NEW_MODES:	0x0000	0000 0000 0000 0000b
//	field name			bits		value and meaning
//	REVISION			0-7			xxx		- These bits read back the current mask revision number
//	POWER_DOWN			8			0		- disable static power down mode.
//	DISABLE_CLOCK_LOSS_DETECT
//						9			0		- This bit should be kept low
//	POWER_DOWN_STATUS	10,11		00		- return power down status as low going strobe.
//	INV_MSB_AOUT		12			0		- do not invert MSB of A-output
//	INV_MSB_BOUT		13			0		- do not invert MSB of B-output
//	INV_MSB_AIN			14			0		- do not invert MSB of A-input
//	INV_MSB_AIN			15			0		- do not invert MSB of B-input

0x0000,  // full rate odd symmetry   

//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Group 2:
//	documenting Half Rate odd symmetry
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   0x248B,0x2E12,0x048F,0x2E12,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000,  
//   half rate odd symmetry    

// APATH_REG0:	0x248b 0010 0100 1000 1011b
0x248b,
//	field name			bits		value and meaning
//		ACCUM			0,1			11		- accumulate 2 sums ( Half rate )
//		UNSIGNED		2			0		- filter cell adder is in signed mode
//		COEF_SEL		3-7			10001	- 0x11 toggle between registers 0 and 1
//		RATE			8,9			11		- 0x03 half rate input
//		NEG_IN			10-12		100		- 0x04 always negate
//		AB_SEL			13			1		- Select input A
//		DELAY_SEL		14,15		00		- no delay

// APATH_REG1:	0x2e12	0010 1110 0001 0010b
0x2e12,
//	field name		bits		value and meaning
//		FEED_BACK		0-4			01000	- 0x08 Full rate even symmetry and A-path cascade mode
//		ANTI_SYM		5			1		- complement the feedback data
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			0		- clear for even symmetry filter
//		REV_DELAY		8-12		01110	- 0x0e decimate and integrate by 2 filters
//		FOR_DELAY		13-15		001		- decimate and interpolate by 2 filters

// BPATH_REG0:	0x048f 0000 0100 1000 1111b
0x048f,
//	field name			bits		value and meaning
//		ACCUM			0,1			00		- don't accumulate (full rate output)
//		UNSIGNED		2			1		- filter cell adder is in unsigned mode
//		COEF_SEL		3-7			11011	- use coefficient register 1
//		RATE			8,9			00		- quarter rate input
//		NEG_IN			10-12		001		- negate even time full rate samples
//		AB_SEL			13			0		- Select input B
//		DELAY_SEL		14,15		00		- set to no delay

// APATH_REG1:	0x2e12	0010 1110 0001 0010b
0x2e12,
//	field name		bits		value and meaning
//		FEED_BACK		0-4			01000	- 0x08 Full rate even symmetry and A-path cascade mode
//		ANTI_SYM		5			1		- complement the feedback data
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			0		- clear for even symmetry filter
//		REV_DELAY		8-12		01110	- 0x0e decimate and integrate by 2 filters
//		FOR_DELAY		13-15		001		- decimate and interpolate by 2 filters

// CASCADE_REG:	0x2000	0010 0000 0000 0000b
0x2000,
//	field name			bits		value and meaning
//		SYNC_COEF		0			0		- Do not synchronize coefficient data with system clock.
//		CASCADE			9-15		1		- //??????????????

// COUNT_reg:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//		CNT				0-15		0		- Preset counter to (16*cnt+15)

// GAIN_REG:	0x00f0	0000 0000 1111 0000b
//		The chip's output gain is set using F an S according to the following formula:
//						(S-20)
//				GAIN = 2  (1+F/16)(DC_GAIN)
//	where:
//			DC_GAIN is the sum of the fillter coefficients.  Unity gain, according to this
//			formula, will map the MSB of the 12 bit input data (A|11 or B|11) into the MSB 
//			of the selected output word.
0x00F0,
//	field name			bits		value and meaning
//	F					0-3			0000	- the 4 bit gain fraction
//	S					4-7			1111	- the 4 bit gain exponent
//	ROUND				8-14	0000 000	- truncate

// OUTPUT_REG:	0x004c	0000 0000 0100 1100b
0x004C,
//	field name			bits		value and meaning
//	NEG_OUT				0,1			00		- Don't Negate
//	24BIT_MODE			2			1		- enable 24 bit mode
//	24BIT_OUT			3			1		- enable 24 bit output mode
//	MUX_MODE			4,5			00		- mux mode is off
//	PATH_ADD			6			1		- adds A-path and B-path results
//	DAV_POLARITY		7			0		- do not invert polarity of data valid (DAV) strobe

//	SNAP_REGA:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_REGB:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_START_REG:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//	TRIGGER				0,1			0		- immediately
//	READ_MODE			2,3			0		- read words
//	ARMED				4			0		- Arm snapshot memory
//	A_DONE				5			0		- Bit goes high when the A-Half snapshot is done
//	B_DONE				6			0		- Bit goes high when the B-Half snapshot is done
//	SYNC_OUT			8,9			00		- Sync_out is SI* delayed by 4 clocks (SYNC_OFF=0)
//	SYNC_OFF			10			0		- Turn sync on.

//	ONE_SHOT:	0x0000	0000 0000 0000 0000b
//			value stored here is unused. and register is write only
0x0000,

//	NEW_MODES:	0x0000	0000 0000 0000 0000b
//	field name			bits		value and meaning
//	REVISION			0-7			xxx		- These bits read back the current mask revision number
//	POWER_DOWN			8			0		- disable static power down mode.
//	DISABLE_CLOCK_LOSS_DETECT
//						9			0		- This bit should be kept low
//	POWER_DOWN_STATUS	10,11		00		- return power down status as low going strobe.
//	INV_MSB_AOUT		12			0		- do not invert MSB of A-output
//	INV_MSB_BOUT		13			0		- do not invert MSB of B-output
//	INV_MSB_AIN			14			0		- do not invert MSB of A-input
//	INV_MSB_AIN			15			0		- do not invert MSB of B-input

0x0000,  // full rate odd symmetry    

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  group 2:
//	documenting Half Rate even symmetry
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//      0x248B,0x2E91,0x048F,0x2E91,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000,
// half rate even symmetry   

// APATH_REG0:	0x248b 0010 0100 1000 1011b
0x248b,
//	field name			bits		value and meaning
//		ACCUM			0,1			11		- accumulate 2 sums ( Half rate )
//		UNSIGNED		2			0		- filter cell adder is in signed mode
//		COEF_SEL		3-7			10001	- 0x11 toggle between registers 0 and 1
//		RATE			8,9			11		- 0x03 half rate input
//		NEG_IN			10-12		100		- 0x04 always negate
//		AB_SEL			13			1		- Select input A
//		DELAY_SEL		14,15		00		- no delay

// APATH_REG1:	0x2e91	0010 1110 1001 0001b
0x2e91,
//	field name		bits		value and meaning
//		FEED_BACK		0-4			10001	- 0x11 Decimate and interpolate by 2 odd symmetry
//		ANTI_SYM		5			0		- do not complement the feedback data
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			1		- set for odd symmetry filter
//		REV_DELAY		8-12		01110	- 0x0e decimate and integrate by 2 filters
//		FOR_DELAY		13-15		001		- decimate and interpolate by 2 filters

// BPATH_REG0:	0x04dc 0000 0100 1101 1100b
0x048f,
//	field name			bits		value and meaning
//		ACCUM			0,1			00		- don't accumulate (full rate output)
//		UNSIGNED		2			1		- filter cell adder is in unsigned mode
//		COEF_SEL		3-7			11011	- use coefficient register 1
//		RATE			8,9			00		- quarter rate input
//		NEG_IN			10-12		001		- negate even time full rate samples
//		AB_SEL			13			0		- Select input B
//		DELAY_SEL		14,15		00		- set to no delay

// BPATH_REG1:	0x2e91	0010 1110 1001 0001b
0x2e91,
//	field name		bits		value and meaning
//		FEED_BACK		0-4			10001	- 0x11 Decimate and interpolate by 2 odd symmetry
//		ANTI_SYM		5			0		- do not complement the feedback data
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			1		- set for odd symmetry filter
//		REV_DELAY		8-12		01110	- 0x0e decimate and integrate by 2 filters
//		FOR_DELAY		13-15		001		- decimate and interpolate by 2 filters

// CASCADE_REG:	0x2000	0010 0000 0000 0000b
0x2000,
//	field name			bits		value and meaning
//		SYNC_COEF		0			0		- Do not synchronize coefficient data with system clock.
//		CASCADE			9-15		1		- //??????????????

// COUNT_reg:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//		CNT				0-15		0		- Preset counter to (16*cnt+15)

// GAIN_REG:	0x00f0	0000 0000 1111 0000b
//		The chip's output gain is set using F an S according to the following formula:
//						(S-20)
//				GAIN = 2  (1+F/16)(DC_GAIN)
//	where:
//			DC_GAIN is the sum of the fillter coefficients.  Unity gain, according to this
//			formula, will map the MSB of the 12 bit input data (A|11 or B|11) into the MSB 
//			of the selected output word.
0x00F0,
//	field name			bits		value and meaning
//	F					0-3			0000	- the 4 bit gain fraction
//	S					4-7			1111	- the 4 bit gain exponent
//	ROUND				8-14	0000 000	- truncate

// OUTPUT_REG:	0x004c	0000 0000 0100 1100b
0x004C,
//	field name			bits		value and meaning
//	NEG_OUT				0,1			00		- Don't Negate
//	24BIT_MODE			2			1		- enable 24 bit mode
//	24BIT_OUT			3			1		- enable 24 bit output mode
//	MUX_MODE			4,5			00		- mux mode is off
//	PATH_ADD			6			1		- adds A-path and B-path results
//	DAV_POLARITY		7			0		- do not invert polarity of data valid (DAV) strobe

//	SNAP_REGA:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_REGB:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_START_REG:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//	TRIGGER				0,1			0		- immediately
//	READ_MODE			2,3			0		- read words
//	ARMED				4			0		- Arm snapshot memory
//	A_DONE				5			0		- Bit goes high when the A-Half snapshot is done
//	B_DONE				6			0		- Bit goes high when the B-Half snapshot is done
//	SYNC_OUT			8,9			00		- Sync_out is SI* delayed by 4 clocks (SYNC_OFF=0)
//	SYNC_OFF			10			0		- Turn sync on.

//	ONE_SHOT:	0x0000	0000 0000 0000 0000b
//			value stored here is unused. and register is write only
0x0000,

//	NEW_MODES:	0x0000	0000 0000 0000 0000b
//	field name			bits		value and meaning
//	REVISION			0-7			xxx		- These bits read back the current mask revision number
//	POWER_DOWN			8			0		- disable static power down mode.
//	DISABLE_CLOCK_LOSS_DETECT
//						9			0		- This bit should be kept low
//	POWER_DOWN_STATUS	10,11		00		- return power down status as low going strobe.
//	INV_MSB_AOUT		12			0		- do not invert MSB of A-output
//	INV_MSB_BOUT		13			0		- do not invert MSB of B-output
//	INV_MSB_AIN			14			0		- do not invert MSB of A-input
//	INV_MSB_AIN			15			0		- do not invert MSB of B-input

0x0000,  // full rate odd symmetry    

//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Group 3:
//	documenting Quarter Rate odd symmetry
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   0x2402,0x0214,0x0406,0x0214,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, 
// quarter rate odd symmetry 
 
// APATH_REG0:	0x2402 0010 0100 0000 0010b
0x2402,
//	field name			bits		value and meaning
//		ACCUM			0,1			10		- 0x02 accumulate 4 sums ( Quarter rate )
//		UNSIGNED		2			0		- filter cell adder is in signed mode
//		COEF_SEL		3-7			00000	- 0x00 cycle through all four registers
//		RATE			8,9			00		- full rate input
//		NEG_IN			10-12		001		- 0x01 negate even full rate samples
//		AB_SEL			13			1		- Select input A
//		DELAY_SEL		14,15		00		- no delay

// APATH_REG1:	0x0214	0000 0010 0001 0010b
0x0214,
//	field name		bits		value and meaning
//		FEED_BACK		0-4			10010	- 0x12 decimate by 2 even symmetry and 
//												decimate by 4 odd symmetry
//		ANTI_SYM		5			0		- do not complement the input data
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			0		- clear for even symmetry filter
//		REV_DELAY		8-12		00010	- 0x02 decimate by 4 and half rate filters
//		FOR_DELAY		13-15		000		- decimate and interpolate by 4 filters

// BPATH_REG0:	0x0406 0000 0100 0000 0110b
0x0406,
//	field name			bits		value and meaning
//		ACCUM			0,1			10		- 0x02 accumulate 4 sums ( Quarter rate )
//		UNSIGNED		2			1		- filter cell adder is in unsigned mode
//		COEF_SEL		3-7			00000	- 0x00 cycle through all four registers
//		RATE			8,9			00		- full rate input
//		NEG_IN			10-12		000		- don't negate
//		AB_SEL			13			0		- Select input B
//		DELAY_SEL		14,15		00		- set to no delay

// APATH_REG1:	0x0214	0000 0010 0001 0010b
0x0214,
//	field name		bits		value and meaning
//		FEED_BACK		0-4			10010	- 0x12 decimate by 2 even symmetry and 
//												decimate by 4 odd symmetry
//		ANTI_SYM		5			0		- do not complement the input data
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			0		- clear for even symmetry filter
//		REV_DELAY		8-12		00010	- 0x02 decimate by 4 and half rate filters
//		FOR_DELAY		13-15		000		- decimate and interpolate by 4 filters

// CASCADE_REG:	0x2000	0010 0000 0000 0000b
0x2000,
//	field name			bits		value and meaning
//		SYNC_COEF		0			0		- Do not synchronize coefficient data with system clock.
//		CASCADE			9-15		1		- //??????????????

// COUNT_reg:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//		CNT				0-15		0		- Preset counter to (16*cnt+15)

// GAIN_REG:	0x00f0	0000 0000 1111 0000b
//		The chip's output gain is set using F an S according to the following formula:
//						(S-20)
//				GAIN = 2  (1+F/16)(DC_GAIN)
//	where:
//			DC_GAIN is the sum of the fillter coefficients.  Unity gain, according to this
//			formula, will map the MSB of the 12 bit input data (A|11 or B|11) into the MSB 
//			of the selected output word.
0x00F0,
//	field name			bits		value and meaning
//	F					0-3			0000	- the 4 bit gain fraction
//	S					4-7			1111	- the 4 bit gain exponent
//	ROUND				8-14	0000 000	- truncate

// OUTPUT_REG:	0x004c	0000 0000 0100 1100b
0x004C,
//	field name			bits		value and meaning
//	NEG_OUT				0,1			00		- Don't Negate
//	24BIT_MODE			2			1		- enable 24 bit mode
//	24BIT_OUT			3			1		- enable 24 bit output mode
//	MUX_MODE			4,5			00		- mux mode is off
//	PATH_ADD			6			1		- adds A-path and B-path results
//	DAV_POLARITY		7			0		- do not invert polarity of data valid (DAV) strobe

//	SNAP_REGA:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_REGB:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_START_REG:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//	TRIGGER				0,1			0		- immediately
//	READ_MODE			2,3			0		- read words
//	ARMED				4			0		- Arm snapshot memory
//	A_DONE				5			0		- Bit goes high when the A-Half snapshot is done
//	B_DONE				6			0		- Bit goes high when the B-Half snapshot is done
//	SYNC_OUT			8,9			00		- Sync_out is SI* delayed by 4 clocks (SYNC_OFF=0)
//	SYNC_OFF			10			0		- Turn sync on.

//	ONE_SHOT:	0x0000	0000 0000 0000 0000b
//			value stored here is unused. and register is write only
0x0000,

//	NEW_MODES:	0x0000	0000 0000 0000 0000b
//	field name			bits		value and meaning
//	REVISION			0-7			xxx		- These bits read back the current mask revision number
//	POWER_DOWN			8			0		- disable static power down mode.
//	DISABLE_CLOCK_LOSS_DETECT
//						9			0		- This bit should be kept low
//	POWER_DOWN_STATUS	10,11		00		- return power down status as low going strobe.
//	INV_MSB_AOUT		12			0		- do not invert MSB of A-output
//	INV_MSB_BOUT		13			0		- do not invert MSB of B-output
//	INV_MSB_AIN			14			0		- do not invert MSB of A-input
//	INV_MSB_AIN			15			0		- do not invert MSB of B-input

0x0000,  // full rate odd symmetry    

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  group 3:
//	documenting Quarter Rate even symmetry
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//   0x2402,0x0292,0x0406,0x0292,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000 
// quarter rate even symmetry

// APATH_REG0:	0x2402 0010 0100 0000 0010b
0x2402,
//	field name			bits		value and meaning
//		ACCUM			0,1			10		- 0x02 accumulate 4 sums ( Quarter rate )
//		UNSIGNED		2			0		- filter cell adder is in signed mode
//		COEF_SEL		3-7			00000	- 0x00 cycle through all four registers
//		RATE			8,9			00		- 0x00 full rate input
//		NEG_IN			10-12		001		- 0x01 negate even full rate samples
//		AB_SEL			13			1		- Select input A
//		DELAY_SEL		14,15		00		- no delay

// APATH_REG1:	0x0292	0000 0010 1001 0010b
0x0292,
//	field name		bits		value and meaning
//		FEED_BACK		0-4			10010	- 0x12 decimate by 2 even symmetry and 
//												decimate by 4 odd symmetry
//		ANTI_SYM		5			0		- do not complement the input data
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			1		- set for odd symmetry filter
//		REV_DELAY		8-12		00010	- 0x02 decimate by 4 and half rate filters
//		FOR_DELAY		13-15		000		- decimate and interpolate by 4 filters

// BPATH_REG0:	0x0406 0000 0100 0000 0110b
0x0406,
//	field name			bits		value and meaning
//		ACCUM			0,1			10		- 0x02 accumulate 4 sums ( Quarter rate )
//		UNSIGNED		2			1		- filter cell adder is in unsigned mode
//		COEF_SEL		3-7			00000	- 0x00 cycle through all four registers
//		RATE			8,9			00		- full rate input
//		NEG_IN			10-12		000		- don't negate
//		AB_SEL			13			0		- Select input B
//		DELAY_SEL		14,15		00		- set to no delay

// APATH_REG1:	0x0292	0000 0010 1001 0010b
0x0292,
//	field name		bits		value and meaning
//		FEED_BACK		0-4			10010	- 0x12 decimate by 2 even symmetry and 
//												decimate by 4 odd symmetry
//		ANTI_SYM		5			0		- do not complement the input data
//		CIN				6			0		- clear carry for symmetric filter
//		ODD_SYM			7			1		- set for odd symmetry filter
//		REV_DELAY		8-12		00010	- 0x02 decimate by 4 and half rate filters
//		FOR_DELAY		13-15		000		- decimate and interpolate by 4 filters

// CASCADE_REG:	0x2000	0010 0000 0000 0000b
0x2000,
//	field name			bits		value and meaning
//		SYNC_COEF		0			0		- Do not synchronize coefficient data with system clock.
//		CASCADE			9-15		1		- //??????????????

// COUNT_reg:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//		CNT				0-15		0		- Preset counter to (16*cnt+15)

// GAIN_REG:	0x00f0	0000 0000 1111 0000b
//		The chip's output gain is set using F an S according to the following formula:
//						(S-20)
//				GAIN = 2  (1+F/16)(DC_GAIN)
//	where:
//			DC_GAIN is the sum of the fillter coefficients.  Unity gain, according to this
//			formula, will map the MSB of the 12 bit input data (A|11 or B|11) into the MSB 
//			of the selected output word.
0x00F0,
//	field name			bits		value and meaning
//	F					0-3			0000	- the 4 bit gain fraction
//	S					4-7			1111	- the 4 bit gain exponent
//	ROUND				8-14	0000 000	- truncate

// OUTPUT_REG:	0x004c	0000 0000 0100 1100b
0x004C,
//	field name			bits		value and meaning
//	NEG_OUT				0,1			00		- Don't Negate
//	24BIT_MODE			2			1		- enable 24 bit mode
//	24BIT_OUT			3			1		- enable 24 bit output mode
//	MUX_MODE			4,5			00		- mux mode is off
//	PATH_ADD			6			1		- adds A-path and B-path results
//	DAV_POLARITY		7			0		- do not invert polarity of data valid (DAV) strobe

//	SNAP_REGA:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_REGB:	0X0000	0000 0000 0000 0000
0x0000,
//	field name			bits		value and meaning
//	SEL_IN				0,1			00		- snap shot source is IB[0:11]
//	SNAP_RATE			2,3			00		- Set smap shot rate to every clock, full rate samples
//	SNAP_DELAY			4-7			0000	- no delay from snapshot trigger
//	SNAP_HOLD			8			0		- start a new snapshot
//	BYTE_MODE			9			0		- use memory as 128 words.

//	SNAP_START_REG:	0x0000	0000 0000 0000 0000b
0x0000,
//	field name			bits		value and meaning
//	TRIGGER				0,1			0		- immediately
//	READ_MODE			2,3			0		- read words
//	ARMED				4			0		- Arm snapshot memory
//	A_DONE				5			0		- Bit goes high when the A-Half snapshot is done
//	B_DONE				6			0		- Bit goes high when the B-Half snapshot is done
//	SYNC_OUT			8,9			00		- Sync_out is SI* delayed by 4 clocks (SYNC_OFF=0)
//	SYNC_OFF			10			0		- Turn sync on.

//	ONE_SHOT:	0x0000	0000 0000 0000 0000b
//			value stored here is unused. and register is write only
0x0000,

//	NEW_MODES:	0x0000	0000 0000 0000 0000b
//	field name			bits		value and meaning
//	REVISION			0-7			xxx		- These bits read back the current mask revision number
//	POWER_DOWN			8			0		- disable static power down mode.
//	DISABLE_CLOCK_LOSS_DETECT
//						9			0		- This bit should be kept low
//	POWER_DOWN_STATUS	10,11		00		- return power down status as low going strobe.
//	INV_MSB_AOUT		12			0		- do not invert MSB of A-output
//	INV_MSB_BOUT		13			0		- do not invert MSB of B-output
//	INV_MSB_AIN			14			0		- do not invert MSB of A-input
//	INV_MSB_AIN			15			0		- do not invert MSB of B-input

0x0000,  // quarter rate odd symmetry    

//Group 1:
//   0x24D8,0x6108,0x04DC,0x6108,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, 
//   0x24D8,0x6181,0x04DC,0x6181,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, 
//Group 2:
//   0x248B,0x2E12,0x048F,0x2E12,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, 
//   0x248B,0x2E91,0x048F,0x2E91,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, 
//Group 3:
//   0x2402,0x0214,0x0406,0x0214,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000, 
//   0x2402,0x0292,0x0406,0x0292,0x2000,0x0000,0x00F0,0x004C,0x0000,0x0000,0x0000,0x0000,0x0000

}; // end of declaration of the FIR chip initializers.
*/