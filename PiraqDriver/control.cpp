/////////////////////////////////////////////////////////////////////////////////////////
// FILE NAME: Control.cpp
//
// DESCRIPTION: CONTROL Class implementation. This class handles the two Status Registers
// and the Timers chips. 
//
/////////////////////////////////////////////////////////////////////////////////////////
#include "Control.h"




/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::CONTROL(unsigned int* pPiraq_BaseAddress)
//
// DESCRIPTION: Constructor that recieves the base address to the Piraq board.
// This address is used to determine the address to the Status Registers and
// the Timers. 
//
// INPUT PARAMETER(s): 
// unsigned int* pPiraq_BaseAddress - Address of Piraq board
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
CONTROL::CONTROL(unsigned int* pPiraq_BaseAddress)
{
	// Set address to Piraq Board
	m_pPiraq_BaseAddress	= pPiraq_BaseAddress;
	
	// Set address of status registers
	pStatus0	= (unsigned short*)(((unsigned char *)m_pPiraq_BaseAddress )+ 0x1400);
	pStatus1	= (unsigned short*)(((unsigned char *)m_pPiraq_BaseAddress )+ 0x1410);

	// Set address of timers
	pTimer0	= (unsigned int *)(((unsigned char *)m_pPiraq_BaseAddress) + 0x1000);
	pTimer1	= (unsigned int *)(((unsigned char *)m_pPiraq_BaseAddress) + 0x1010);

	// Reset Dsp to put it into a known state
	ResetPiraqBoard();

	// Turn the PC LED off
	m_LEDState = 0; // used to remember the state of the LED
	UnSetBit_StatusRegister0(STAT0_PCLED);

}





/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::~CONTROL()
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
CONTROL::~CONTROL()
{
}





/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::SetValue_StatusRegister0(unsigned short usValue)
//
// DESCRIPTION: Sets the entire Status Register 0 to this value 
//
// INPUT PARAMETER(s):
// unsigned short usValue - Value to set register to
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
void CONTROL::SetValue_StatusRegister0(unsigned short usValue)
{
	// Store Value for later
	StatusRegister0Value = usValue;
	
	// Set register with value;
	*pStatus0 = StatusRegister0Value;
}





/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::SetBit_StatusRegister0(unsigned short usBit)
//
// DESCRIPTION: Sets only this bit in Status Register 0 
//
// INPUT PARAMETER(s): 
// unsigned short usBit - Bit to set such as 0x0002 would put a 1 in the second bit
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
void CONTROL::SetBit_StatusRegister0(unsigned short usBit)
{
	// Add bit(s) to stored value from last time
	StatusRegister0Value = StatusRegister0Value | usBit;

	// Set register with new value
	*pStatus0 = StatusRegister0Value;

	// Note that some bits are write only. 
	// So one can not read the value, OR in the bits, and re-write the value
}






/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::UnSetBit_StatusRegister0(unsigned short usBit)
//
// DESCRIPTION: Removes only this bit in Status Register 0  
//
// INPUT PARAMETER(s): 
// unsigned short usBit - Bit to remove such as 0x0002 would put a 0 in the second bit
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
void CONTROL::UnSetBit_StatusRegister0(unsigned short usBit)
{
	// Remove bit(s) to stored value from last time
	StatusRegister0Value = StatusRegister0Value & ~usBit;
		
	// Set register with new value
	*pStatus0 = StatusRegister0Value;

	// Note that some bits are write only. 
	// So one can not read the value, OR in the bits, and re-write the value

}






/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::SetValue_StatusRegister1(unsigned short usValue)
//
// DESCRIPTION: Sets the entire Status Register 1 to this value 
//
// INPUT PARAMETER(s):
// unsigned short usValue - Value to set register to
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
void CONTROL::SetValue_StatusRegister1(unsigned short usValue)
{
	// Store Value for later
	StatusRegister1Value = usValue;
	
	// Set register with value;
//!!!!!!0???????	*pStatus1 = StatusRegister0Value;
	*pStatus1 = StatusRegister1Value;
}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::SetBit_StatusRegister1(unsigned short usBit)
//
// DESCRIPTION: Sets only this bit in Status Register 1 
//
// INPUT PARAMETER(s): 
// unsigned short usBit - Bit to set such as 0x0002 would put a 1 in the second bit
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
void CONTROL::SetBit_StatusRegister1(unsigned short usBit)
{
	// Add bit(s) to stored value from last time
	StatusRegister1Value = StatusRegister1Value | usBit;

	// Set register with new value
	*pStatus1 = StatusRegister1Value;

	// Note that some bits are write only. 
	// So one can not read the value, OR in the bits, and re-write the value
}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::UnSetBit_StatusRegister1(unsigned short usBit)
//
// DESCRIPTION: Removes only this bit in Status Register 1  
//
// INPUT PARAMETER(s): 
// unsigned short usBit - Bit to remove such as 0x0002 would put a 0 in the second bit
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
void CONTROL::UnSetBit_StatusRegister1(unsigned short usBit)
{
	// Remove bit(s) from stored value from last time
	StatusRegister1Value = StatusRegister1Value & ~usBit;
		
	// Set register with new value
	*pStatus1 = StatusRegister1Value;

	// Note that some bits are write only. 
	// So one can not read the value, OR in the bits, and re-write the value
}








/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::ResetDsp()
//
// DESCRIPTION: Performs a software reset. This should put the DSP in a known state 
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
void CONTROL::ResetPiraqBoard()
{

	// Put timers and PLDs into a known state
	InitTimers();

	// Put DSP into a reset state
	UnSetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);

	// Take DSP out of the reset state
	SetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);

}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::InteruptDSP()
//
// DESCRIPTION: Interupts the DSP 
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
void CONTROL::InteruptDSP()
{
	UnSetBit_StatusRegister1(STAT1_PCI_INT);
	SetBit_StatusRegister1(STAT1_PCI_INT);

	// Once the STAT1_PCI_INT bit is set, the DSP will continue
	// with it's code. If this is the semaphore stating that the
	// parameters are set, then it will start to read in the parameters
	// from host memory. If the Host application attempts to read or 
	// write to the registers, LEDs, Fifo's, etc on the Piraq board
	// while the DSP is accessing host memory via the PLX PCI chip, then
	// a dead lock can occure.
}

	


/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::ToggleLED()
//
// DESCRIPTION: Toggles the PC LED on and off based on the state. 
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
void CONTROL::ToggleLED()
{

	//	Turn the PC LED on the board  on and off by alternating 
	//	between one and zero in the setting.
	if (m_LEDState)
	{
		UnSetBit_StatusRegister0(STAT0_PCLED);
		m_LEDState = 0;
	}
	else
	{
		SetBit_StatusRegister0(STAT0_PCLED);
		m_LEDState = 1;
	}


}

	






/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::StopDsp()
//
// DESCRIPTION: Stops the DSP by stopping the Timers and Reseting the DSP 
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
void CONTROL::StopDsp()
{
	// Put DSP into a reset state
	UnSetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);

	// Take DSP out of the reset state
	SetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);
}




//****************** Timers **********************************



/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::InitTimers()
//
// DESCRIPTION: Puts timers into a know state
//
// INPUT PARAMETER(s): 
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
void CONTROL::InitTimers()
{
	// Stop Timers
	StopTimers();

	// Set TimerChip0 - Counter0 into known state
	SetTimerRegister(Counter_0,HStrobe,2,pTimer0);  

	// Set TimerChip0 - Counter1 into known state
	SetTimerRegister(Counter_1,HStrobe,2,pTimer0);  

	// Set TimerChip0 - Counter2 into known state
	SetTimerRegister(Counter_2,HStrobe,2,pTimer0);  

	// Set TimerChip1 - Counter0 into known state
	SetTimerRegister(Counter_0,HStrobe,2,pTimer1); 
 
	// Set TimerChip1 - Counter1 into known state
	SetTimerRegister(Counter_1,HStrobe,2,pTimer1); 
 
	// Set TimerChip1 - Counter2 into known state
	SetTimerRegister(Counter_2,HStrobe,2,pTimer1); 
 
 	// Set Status Register 0 with timing values 
	// Do not do a SW_Reset
	SetValue_StatusRegister0(0x1);

	// Set Status Register 1 with timing values
	SetValue_StatusRegister1(0x0000);

  return;
}




/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::StartTimers()
//
// DESCRIPTION: Starts the Timers 
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
void CONTROL::StartTimers()
{

	// Take Timer out of reset by seting bit 1 (TRESET) to 1
	SetBit_StatusRegister0(STAT0_TRESET);
}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::StopTimers()
//
// DESCRIPTION: Stops the Timers 
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
void CONTROL::StopTimers()
{
	// Put Timer into reset by seting bit 1 (TRESET) to 0
	UnSetBit_StatusRegister0(STAT0_TRESET);


	// Wait for all timers to time-out
	Sleep(30);       

}








/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::SetUpTimers(float fFrequencyHz, long lTransmitPulseWidthNs,long lPreTRNs,
//												 long lTimeDelayToFirstGateNs, long lGateSpacingNs, long lNumRangeGates)
//
// DESCRIPTION: Sets up the Timers by actually setting the correct values into the timer chips
// and setting the correct values into the status registers
// 
// program the timer for different modes with
//
//	mode:           0 = continuous mode, 1 = trigger mode, 2 = sync mode
//	gate0mode:      0 = regular gate 0, 1 = expanded gate 0
//	gatecounts:     gate spacing in 10 MHz counts
//	numbergates:    number of range gates
//	timecounts:     modes 1 and 2: delay (in 10 MHz counts) after trigger
//	before sampling gates
//	mode 0: pulse repetition interval in 10 MHz counts
//
//	For sync mode (mode 0), delaycounts defines the sync time.  
//	The prt delay must be reprogrammed after the FIRST signal is detected.  
//
//
// INPUT PARAMETER(s): 
// float fFrequencyHz - Radar Frequency in Hz
// long lTransmitPulseWidthNs - Width of the Transmit pulse in nanoseconds
// long lPreTRNs - Time from start of Sync pulse to start of Transmit pulse in nanoseconds
// long lTimeDelayToFirstGateNs - Time from end of Transmit pulse to start of first sampling gate in nanoseconds
// long lGateSpacingNs - Time from one sampling gate to the next in nanoseconds
// long lNumRangeGates - Number of sampling gates
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
int CONTROL::SetUpTimers(float fFrequencyHz, long lTransmitPulseWidthNs,long lPreBlankNs,
												 long lTimeDelayToFirstGateNs, long lGateSpacingNs, long lNumRangeGates, 
												 int iTimerDivide, int iGateSpacingInClockCycles, long lNumCodeBits)
{
	// Stop Timers
	StopTimers();

	if(lNumCodeBits == 0)
	{
		lNumCodeBits = 1;
	}

	// *** Set DELAY - Which is the time from the Sync pulse to the first Sample gate.
	// Calculate the number of clock cycles to just before the first gate starting at the 
	// rising edge of the sync pulse. Notice that one GateSpacing has been removed from this
	// time. This is becuase the PLD logic takes one full GateSpaceing of time before putting 
	// out the first gate.
	// Divide by iTimerDivide because the Timer chip can not run at 48MHz.
	// The timer clock is 48Mhz/iTimerDivide.
	// Truncate so that it falls just short of the first gate
	m_iCountsPerDelay = ((int)((lPreBlankNs + (lNumCodeBits * lTransmitPulseWidthNs) + lTimeDelayToFirstGateNs - lGateSpacingNs)*fFrequencyHz*1e-9)/iTimerDivide);
	// Set TimerChip0 - Counter2 with Delay to Gates
	SetTimerRegister(Counter_2,HStrobe,m_iCountsPerDelay,pTimer0);  

	// *** Set 'Number Of Gates' - Which is the time to the end of Sample gates.
	// The time starts when the DELAY timer triggers (1 GateSpacing before the first Sample gate)
	// Calculate the number of clock cycles it takes to include all gates.
	// Add .5 gates to correctly place the EOF that happens after each set of gates.
	// Divide by iTimerDivide because the Timer chip can not run at 48MHz.
	// The timer clock is 48Mhz/iTimerDivide.
	// Round up so that it includeds all gates
	m_iCountsForAllGates = (int)ceil(( iGateSpacingInClockCycles * (lNumRangeGates + .5) )/iTimerDivide);
	// Set TimerChip1 - Counter1 with time of all Gates
	SetTimerRegister(Counter_1,HStrobe,m_iCountsForAllGates,pTimer1); 
 

	// Set the Timer Divide ratio into the PLDs
	// The PLDs divide the input clock by the iTimerDivide ratio.
	// The IPP length must be an even multiple of this divide ratio.
	// Thus the PLDs have to be set to the same ratio or the input Sync pulse 
	// will not come out of the PLD as the 'Trigger' pulse.
	
	// FINISH - 1/6th does not work because we can not figure out how to set GateClk

	if(iTimerDivide == 8)
	{
		// Set bits 0 & 1 to 0
		m_iSpare = 0x0;
	}
	else if (iTimerDivide == 6)
	{
		// Set bit 0 to 1 and bit 1 to 1
		m_iSpare = STAT1_SPARE0;
	}
	else
	{
		// Default to 6
		// Set bit 0 to 1 and bit 1 to 1
		m_iSpare = STAT1_SPARE0;
	}

	// Depending on the lTransmit Pulse Width, the FIR is either in /1 , /2 or /4 mode 
  if ( lTransmitPulseWidthNs < 1000 /*ns*/ )    
  { 
		// Set counter to divide input ADC sample clock by 0
		m_iSpare = m_iSpare | 0x000;

		// *** Set GLEN0 - GLEN3 - Which is the number of clock cycles per Gate
		// The count is for only one half of a full Gate Spacing.
		// Because of this, it has already been made a multiple of 2 clock cycles.
		// Subtract 1 from the count because the PLD uses 1 clock cycle to reload the counters, once for each GLEN.
		// Shift left to 9th bit location to put into the correct place in the registers.	
		m_iGateLengthCount = ((iGateSpacingInClockCycles/2) - 1)  << 9;

		// Store value for future reference
		m_iCountsPerGatePulse = m_iGateLengthCount >> 9;

	}
	else if (lTransmitPulseWidthNs < 2000 /*ns*/ )	
	{ 
		// Set counter to divide input ADC sample clock by 2 
		m_iSpare = m_iSpare | STAT1_SPARE2;

		// *** Set GLEN0 - GLEN3 - Which is the number of clock cycles per Gate
		// The count is for only one half of a full Gate Spacing.
		// Because of this, it has already been made a multiple of 2 clock cycles.
		// It has also already been made a multiple of 4 clock cycles because we are also dividing the clock in half.
		// Subtract 2 from the count because the PLD uses 1 divided clock cycle to reload the counters, once for each GLEN.
		// Shift left to 9th bit location to put into the correct place in the registers.	
		// By shifting left only 8 bits we are dividing the count by 2 because the clock has been divided in half.	
		m_iGateLengthCount = ((iGateSpacingInClockCycles/2) - 2)  << 8;  

		// Store value for future reference
		m_iCountsPerGatePulse = m_iGateLengthCount >> 9;
	}

  else												
  {
		// Set counter to divide input ADC sample clock by 4 
		m_iSpare = m_iSpare | STAT1_SPARE3;

		// *** Set GLEN0 - GLEN3 - Which is the number of clock cycles per Gate
		// The count is for only one half of a full Gate Spacing.
		// Because of this, it has already been made a multiple of 2 clock cycles.
		// It has also already been made a multiple of 8 clock cycles because we are also dividing the clock by 4.
		// Subtract 4 from the count because the PLD uses 1 divided clock cycle to reload the counters, once for each GLEN.
		// Shift left to 9th bit location to put into the correct place in the registers.	
		// By shifting left only 7 bits we are dividing the count by 4 because the clock has been divided by 4.	
		m_iGateLengthCount = ((iGateSpacingInClockCycles/2) - 4) << 7;  
	
		// Store value for future reference
		m_iCountsPerGatePulse = m_iGateLengthCount >> 9;
	}
		

	//*** Set Status Register 0 with timing values

	// First clear all bits except for the software reset (SW_RESET)
	SetValue_StatusRegister0(STAT0_SW_RESET);

	// Set bit 2 - TMODE to 0 because ????
	UnSetBit_StatusRegister0(STAT0_TMODE); 

	// Set bit 3 - DELAY_SIGN to 0 to delay the trigger pulse by the count in Timer2 in Chip 1 CNT02 - DELAY
  UnSetBit_StatusRegister0(STAT0_DELAY_SIGN);	

	// Set bit 4 - EVEN_TRIG to 1 to output all even triggers
  SetBit_StatusRegister0(STAT0_EVEN_TRIG);	

	// Set bit 5 - ODD_TRG to 1 to output all odd triggers
  SetBit_StatusRegister0(STAT0_ODD_TRIG);	

	// Set bit 6 - EVEN_TP to 0 because we are not using the Test Pulse
  UnSetBit_StatusRegister0(STAT0_EVEN_TP);	

	// Set bit 7 - ODD_TP to 0 to because we are not using the Test Pulse
  UnSetBit_StatusRegister0(STAT0_ODD_TP);	

	// Set bits 9,10,11,12,13 (GLEN0-4) to the length of one Sample pulse in clock cycles.
	SetBit_StatusRegister0(m_iGateLengthCount);	 

	// Set bit 14 - GATE0MODE to 0 to because ????
  UnSetBit_StatusRegister0(STAT0_GATE0MODE);	

	// Set bit 15 - SAMPLE_CTRL to 0 to ????
  UnSetBit_StatusRegister0(STAT0_SAMPLE_CTRL);	

	
	//*** Set Status Register 1 with timing values

	// First clear all entries 
	SetValue_StatusRegister1(0x0000);

	// Set bit 0 - PLL_CLOCK to 0 because we are not using Phase Lock Loop
  UnSetBit_StatusRegister1(STAT1_PLL_CLOCK);	

	// Set bit 1 - PLL_LE to 0 because we are not using Phase Lock Loop
  UnSetBit_StatusRegister1(STAT1_PLL_LE);	

	// Set bit 2 - PLL_DATA to 0 because we are not using Phase Lock Loop
  UnSetBit_StatusRegister1(STAT1_PLL_DATA);	

	// Set bit 3 - WATCHDOG to 0 because we are not using the watchdog timer
  UnSetBit_StatusRegister1(STAT1_WATCHDOG);	

	// Set bit 4 - PCI_INT to 0 because ?
  UnSetBit_StatusRegister1(STAT1_PCI_INT);	

	// Set bit 5 - EXTTRIGEN to 0 because we are not putting an input trigger signal to BTRGIN(U4 pin 22)
  UnSetBit_StatusRegister1(STAT1_EXTTRIGEN);	

	// Set bit 6 - FIRST_TRIG - READ ONLY

	// Set bit 7 - PHASELOCK - READ ONLY

	// Set bits 8,9,10,11 - Spare0-3 - WRITE ONLY
	// Set counter to divide input ADC sample clock by 4 for pulsewidth count
	// This sets the decimation rate
	SetBit_StatusRegister1(m_iSpare); 

	
/* TEMPORARY ************************

	// This allows the Sync pulse to come in through the BTrigIn connector,
	// which is the top SMA connector on the front of the board. This
	// is being done to get around a hardware issue in the PDL that has to 
	// do with lining the Sync Pulse up on the correct phase of A0,A1 & A2,
	// which are divided clocks internal to the PDL.
	SetBit_StatusRegister1(STAT1_EXTTRIGEN);
	SetBit_StatusRegister0(STAT0_TMODE); 

	// Set PRT1 & PRT2 to 1 count. 
	// The SetTimerRegister function subtracts 1 off of the  total count to reload
	SetTimerRegister(Counter_0,HStrobe,2,pTimer0); 
	SetTimerRegister(Counter_1,HStrobe,2,pTimer0); 
	
	// Reduce the delay count by 2 because the Sync signal has to run through 
	// two extra counters called PRT1 & PRT2. .
	m_iCountsPerDelay = m_iCountsPerDelay - 2;

	// Reduce the delay count by 1 to account for the delay through the External trigger logic
	m_iCountsPerDelay = m_iCountsPerDelay - 1;

	// Reset TimerChip0 - Counter2 with Delay to Gates
	SetTimerRegister(Counter_2,HStrobe,m_iCountsPerDelay,pTimer0);  

// END TEMPORARY ************************/
		

  return 0;
}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::SetTimerRegister(int iCounter,int iTimerMode,int iCount,unsigned int *pTimerRegister)
//
// DESCRIPTION: Sets the Timer chips. First it writes the control word. Next it writes
// the upper 16 bits of the count. Then it writes the lower 16 bits of the count. The Timers
// are clocked at 1/4th of the radar frequncy clock. This divide by 4 happens in the PALs.
//
// INPUT PARAMETER(s): 
// int iCounter - Determines which of the three timers on one chip is being set
// int iTimerMode - Determines the timer mode
// int iCount - The number of counts that the timer will count in (clock cycles/4).  
// unsigned int *pTimerRegister
//
// Write Counter Control Word
// bit 0 always = 0 = 16 bit binary counter
// bit 1,2&3 = (iTimerMode * 2) = (iTimerMode = 5) = Hardware Triggered Strobe
// bit 4&5 = 0x30 = read/write ls bytes first then ms byte
// bit 6&7 = (iCounter * 0x40) = selects counter. 
//
// Mode: 5 = HStrobe
// Hardware Triggered Strobe	(Retriggerable)
// OUT will initially be high.  Counting is triggered by a rising edge of GATE.
// When the initial count has expired, OUT will go low for one CLK pulse and 
// then go high again.
// After wirthing the Control Word and initial count, the counter will not be
// loaded until the CLK pulse after a trigger.  This CLK pulse does not decrement
// the count, so for an initial count of N, OUT doe not strobe low until N+1 CLK
// pulses after a trigger.
// A trigger rusults in the Coounter being loaded with the initial count on the 
// next CLK pulse.  The counting sequence is retirggerable.  OUT will not strobe
// low for N+1 CLK pulses after any trigger.  GATE has no effect on OUT.
// If a new count is written during counting, the current counting sequence will
// not be affected.  If a trigger occurs after the new count is written but before
// the current count expires, the Counter will be loaded with the new count on the
// next CLK pulse and counting will continue from there.
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
void CONTROL::SetTimerRegister(int iCounter,int iTimerMode,int iCount,unsigned int *pTimerRegister)
{

	// Subtract one count because the Timer uses one count to load and start the counter
	iCount = iCount-1;

	volatile short* pTimerOffset;

	Sleep(1);

	// Write Counter Control Word
	// bit 0 always = 0 = 16 bit binary counter
	// bit 1,2&3 = (iTimerMode * 2) = (iTimerMode = 5) = Hardware Triggered Strobe
	// bit 4&5 = 0x30 = read/write ls bytes first then ms byte
	// bit 6&7 = (iCounter * 0x40) = selects counter. 
	pTimerOffset =((volatile short *)pTimerRegister + 0x3);
	*pTimerOffset	= ((iCounter * 0x40) + 0x30 + (iTimerMode * 2));
 

	Sleep(1);

	// Write Counter Least Significant Byte
	pTimerOffset =(((volatile short *)pTimerRegister + iCounter));
	*pTimerOffset	= iCount;


	Sleep(1);

	// Write Counter Most Significant Byte
	pTimerOffset =((volatile short *)pTimerRegister + iCounter);
	*pTimerOffset	= (iCount >> 8);


}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: CONTROL::ReturnTimerValues(unsigned int* piCountsPerDelay,	unsigned int* piCountsForAllGates,
//																unsigned int* piCountsPerGatePulse,unsigned short* piSpare)
//
// DESCRIPTION: Returns the values last used to set up the timers for diagnostics 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): 
// unsigned int* piCountsPerDelay - Time in clock cycles from start of Sync to start of first sampling gate
// unsigned int* piCountsForAllGates - Time in clock cycles for all sampling gates to happen
// unsigned int* piCountsPerGatePulse - Time in clock cycles from the start of one sampling gate to the start of the next
// unsigned short* piSpare - Value in the upper 16 bits of Status Register 1 that sets the decimation rate
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
void CONTROL::ReturnTimerValues(unsigned int* piCountsPerDelay,	unsigned int* piCountsForAllGates,
																unsigned int* piCountsPerGatePulse,unsigned short* piStatus0, unsigned short* piStatus1)
{
	*piCountsPerDelay = m_iCountsPerDelay;
	*piCountsForAllGates = m_iCountsForAllGates;
	*piCountsPerGatePulse = m_iCountsPerGatePulse;
	*piStatus0 = *pStatus0;
	*piStatus1 = *pStatus1;
}



   














	

