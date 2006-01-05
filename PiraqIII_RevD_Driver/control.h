/////////////////////////////////////////////////////////////////////////////////////////
// FILE NAME: Control.h
//
// DESCRIPTION: CONTROL Class implementation. This class handles the two Status Registers
// and the Timers chips. 
//
/////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

// the bits of the status register 
#define STAT0_SW_RESET    0x0001
#define STAT0_TRESET      0x0002
#define STAT0_TMODE       0x0004
#define STAT0_DELAY_SIGN  0x0008
#define STAT0_EVEN_TRIG   0x0010
#define STAT0_ODD_TRIG    0x0020
#define STAT0_EVEN_TP     0x0040
#define STAT0_ODD_TP      0x0080
#define STAT0_PCLED       0x0100
#define STAT0_GLEN0       0x0200
#define STAT0_GLEN1       0x0400
#define STAT0_GLEN2       0x0800
#define STAT0_GLEN3       0x1000
#define STAT0_GLEN4       0x2000
#define STAT0_GATE0MODE   0x4000
#define STAT0_SAMPLE_CTRL 0x8000

#define STAT1_PLL_CLOCK   0x0001
#define STAT1_PLL_LE      0x0002
#define STAT1_PLL_DATA    0x0004
#define STAT1_WATCHDOG    0x0008
#define STAT1_PCI_INT     0x0010
#define STAT1_EXTTRIGEN   0x0020
#define STAT1_FIRST_TRIG  0x0040
#define STAT1_PHASELOCK   0x0080
#define STAT1_SPARE0      0x0100
#define STAT1_SPARE1      0x0200
#define STAT1_SPARE2      0x0400
#define STAT1_SPARE3      0x0800



#define Counter_0	0
#define Counter_1	1
#define Counter_2	2

#define HStrobe	5
#define export __declspec(dllexport)

class export CONTROL
{
	private:
		unsigned long lReturnVal;
		unsigned int m_iCountsPerGatePulse;
		unsigned int m_iCountsPerDelay;
		unsigned int m_iCountsForAllGates;
		unsigned int m_iGateLengthCount;
		unsigned short m_iSpare;

		unsigned int*		m_pPiraq_BaseAddress;

		int m_LEDState;
		unsigned short   *pStatus0,*pStatus1;

		unsigned short		StatusRegister0Value;
		unsigned short		StatusRegister1Value;

		// Timers
		unsigned int		 *pTimer0;
		unsigned int		 *pTimer1;



	public:
		CONTROL(unsigned int *pPiraq_BaseAddress);
		~CONTROL();

		void ResetPiraqBoard();
		void ToggleLED();
		void InteruptDSP();
		void StopDsp();
		void InitTimers();
		void StartTimers();
		void StopTimers();

		// Timers
		int SetUpTimers(float fFrequencyHz, long lTransmitPulseWidthNs,long lPreBlankNs,
										long lTimeDelayToFirstGateNs, long lGateSpacingNs, long lNumRangeGates, 
										int iTimerDivide, int iGateSpacingInClockCycles, long lNumCodeBits);

		void SetTimerRegister(int iCounter,int iTimerMode,int iCount,unsigned int *pTimerRegister);

		void CONTROL::ReturnTimerValues(unsigned int* piCountsPerDelay,	unsigned int* piCountsForAllGates,
																		unsigned int* piCountsPerGatePulse,unsigned short* piStatus0,unsigned short* piStatus1);
		
		void SetValue_StatusRegister0		(unsigned short usValue);
		void SetBit_StatusRegister0			(unsigned short usBit);
		void UnSetBit_StatusRegister0		(unsigned short usBit);
		void SetValue_StatusRegister1		(unsigned short usValue);
		void SetBit_StatusRegister1			(unsigned short usBit);
		void UnSetBit_StatusRegister1		(unsigned short usBit);
		unsigned short GetValue_StatusRegister0(){return(StatusRegister0Value);}
		unsigned short GetValue_StatusRegister1(){return(StatusRegister1Value);}
		unsigned short GetBit_StatusRegister0(unsigned short bit){return(StatusRegister0Value & bit);}
		unsigned short GetBit_StatusRegister1(unsigned short bit){return(StatusRegister1Value & bit);}
		unsigned short *GetTimer(){return (unsigned short *)pTimer0;}
};


