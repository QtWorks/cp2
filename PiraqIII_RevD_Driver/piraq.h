/////////////////////////////////////////////////////////////////////////////////////////
// FILE NAME: Piraq.h
//
// DESCRIPTION: PIRAQ Class implementation. This class handles the exposed APIs that
// the host application such as LAPXM would use.
// to the DSP.
//
/////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"

// Offset to the address of the CommonBuferMemory.
// This memory is used to pass data from the DSP to the Windows application
#define	DMPBAM		 0x28

// Offset to length in bytes of CommonBuferMemory.
#define	DMRR			 0x1c 

// Offset to remap of DSP to common buffer.
// The DSP writes to this location to write to the common buffer
#define	DMLBAM		 0x20 

// Offset to PCI mailbox registers
#define PIRAQ_MAILBOX0 0x40
#define PIRAQ_MAILBOX1 0x44
#define PIRAQ_MAILBOX2 0x48
#define PIRAQ_MAILBOX3 0x4c
#define PIRAQ_MAILBOX4 0x50
#define PIRAQ_MAILBOX5 0x54
#define PIRAQ_MAILBOX6 0x58
#define PIRAQ_MAILBOX7 0x5c

#define PARAMETEROFFSET 0x00
#define SEMAPHOREOFFSET 0x90
#define NUMCODEBITS 0
#define NUMCOHERENTINTEGRATIONS 1
#define NUMRANGEGATES 2
#define NUMPOINTSINSPECTRUM 3
#define NUMRECEIVERS 4
#define FLIP 5

// CP2: 3 modes of data transfer from PIRAQ to host: first 2 are diagnostic
//		!note: same set is defined in proto.h
#define	SEND_CHA		0		// send CHA
#define	SEND_CHB		1		// send CHB
#define	SEND_COMBINED	2		// execute dynamic-range extension algorithm; send resulting combined data
// PIRAQ test-sinusoid adjustments
// 4 PIRAQ test-sinusoid adjustments
#define	INCREMENT_TEST_SINUSIOD_COARSE	4
#define	INCREMENT_TEST_SINUSIOD_FINE	8
#define	DECREMENT_TEST_SINUSIOD_COARSE	12
#define	DECREMENT_TEST_SINUSIOD_FINE	16

#define export __declspec(dllexport)


class export PIRAQ 
{
	private:
		unsigned long lReturnValue;
		char m_cErrorString[256];

		void			*m_pPLX;
		void			*m_pControl;
		void			*m_pHpib;
		void			*m_pFirFilter;
		void			*m_pPCI_Memory;


		unsigned int		*m_pPiraq_BaseAddress;
		unsigned long		*m_pPLX_BaseAddress;
		unsigned long		*m_pPC_BaseAddress;
		unsigned long		*m_pCommonBufferAddressOn8MegBounderyPhysical;
		unsigned long		*m_pCommonBufferAddressOn8MegBounderyUser;
		unsigned long		m_lCommonBufferLength;
		unsigned long		*m_pSemaphores;

		

	public:
		
		// stuff for backward compatibility /////////////////
		UINT64       oldbeamnum,beamnum,pulsenum;
		int             eofstatus,hitcount;
		float           numdiscrim,dendiscrim,g0invmag;
		unsigned short *timer;
		unsigned short	*FIRCH1_I;
		unsigned short	*FIRCH1_Q;
		unsigned short	*FIRCH2_I;
		unsigned short	*FIRCH2_Q;
		/////////////////////////////////////////////////////
	
		PIRAQ();
		~PIRAQ();
		
		void	SetErrorString(char* pErrorString);
		void	GetErrorString(char* pErrorString);
		int		Init( unsigned short shVendorID,  unsigned short shDeviceID );
		void	GetCommonBufferMemoryPointer(unsigned long **pCommonMemoryAddress, unsigned long *plLength);
		void	GetSemaphorePointer(unsigned long **pSemaphore);
		void	ToggleLED();
		void	ReadEPROM(unsigned int * pEPROM);
		int		PCMemoryTest(unsigned int StartValue);


		void	ResetPiraq();
		int		LoadDspCode(char* pFilePathAndName);
		void	LoadParameters(float fFrequencyHz, long lTransmitPulseWidthNs,
													long lPreTRNs, long lTimeDelayToFirstGateNs,
													long lGateSpacingNs, long lNumRangeGates,
													long lNumCodeBits, long lNumCoherentIntegrations,
													long lNumPointsInSpectrum, long lNumReceivers,
													int iTimerDivide, int iFilterType, long lFlip);
// !!!temporary until LoadParameters fully integrated. mp 11-21-02
void	LoadFIRParameters(float sp_fFrequencyHz, long sp_lTransmitPulseWidthNs, long sp_lNumCodeBits); 
		void	ReturnTimerValues(unsigned int *piCountsPerDelay,	unsigned int *piCountsForAllGates,
													 unsigned int *piCountsPerGatePulse,unsigned short* piStatus0,unsigned short* piStatus1);
		void	ReturnFilterValues(unsigned short	*pFIR, unsigned int *piCount);
		void	StartDsp();
		void	StopDsp();
		long	SemaSet(int iSemaphore);
		long	SemaWait(int iSemaphore, int iTimeOutInMiliSeconds);
		void	SetPMACAntennaDPRAMAddress(unsigned short * PMACAntennaDPRAMAddress); 
		unsigned short *	GetPMACAntennaDPRAMAddress(void); 
		void	SetCP2PIRAQTestAction(unsigned short PIRAQTestAction); 
		class CONTROL *GetControl(){ return((class CONTROL *)m_pControl);}
		class HPIB *GetHPIB(){ return((class HPIB *)m_pHpib);}
		unsigned long *GetBuffer(){return(m_pCommonBufferAddressOn8MegBounderyUser);}
		class FIRFILTER *GetFilter(){ return((class FIRFILTER *)m_pFirFilter);}
		unsigned long *GetRegisterBase(){return m_pPLX_BaseAddress;}
}; 

