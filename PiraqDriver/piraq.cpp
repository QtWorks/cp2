/////////////////////////////////////////////////////////////////////////////////////////
// FILE NAME: Piraq.cpp
//
// DESCRIPTION: PIRAQ Class implementation. This class handles the exposed APIs that
// the host application such as LAPXM would use.
//
// PiraqIII Errata

// The option of using a divide by 6 instead of divide by 8 clock for the timers 
// does not work because register bits 8 & 9 (Spare0 & Spare1) do not run to the second PLD. 
// For this reason the clock (GateClk) to some of the timers (Dealy, TP Width, No of Gates Chan1,
// and No of Gates Chan2) are always set to divide by 8.
//
// The InterPulsePeriod must be a multiple of 8 since the timers use a divide by 8 clock.
//
// The Read & Write timing specified in the DSP code for access to SBRAM, SDRAM, Registers, 
// LEDS, PLX chip, Host memory, etc may not be set correctly for a worse case condition. 
// Thus this code may not function correctly in every PC.
//
// Data is slightly corrupted when DMAing from SDRAM to PC Host Memory.  
// Use a direct copy instead.
//
// Since the PiraqIII board does not support burst reads and writes, a direct copy 
// from the PiraqIII board to PC Host Memory is just as fast as DMA.
//
// A DEADLOCK conditions occurs when the DSP is accessing the PC Host Memory via 
// the PLX PCI chip while the Host Application is accessing the PiraqIII registers, 
// LEDs, Memory, Etc. This is a stated problem in the PLX PCI chip documentation with 
// no work around. For this reason, one has to be careful in the code to never allow 
// this condition to happen. The deadlock will occur occasionally making it hard to track.
//
// The programmable Altera chip does not contain logic to arbitrate the bus when the DSP 
// is trying to access the PLX PCI chip registers such as mailboxes while the Host Application 
// is trying to access the PiraqIII registers, LEDS, memory, etc. In this situation, 
// addresses and data can be corrupted and the board can lock up. Once again, one must be careful 
// to avoid this condition.
//
//
// WRITTEN BY: Coy Chanders - Coy@Chanders.Com - 03-14-02
/////////////////////////////////////////////////////////////////////////////////////////
#include "Piraq.h"
#include "PLX.h"
#include "Control.h"
#include "Hpib.h"
#include "FirFilters.h"


/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::PIRAQ()
//
// DESCRIPTION: Constructor 
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
PIRAQ::PIRAQ()
{
}






/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::~PIRAQ()
//
// DESCRIPTION: Destructor - Deletes instances of the other classed that were used 
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
PIRAQ::~PIRAQ()
{
	delete m_pPLX;
	delete m_pControl;
	delete m_pHpib;
	delete m_pFirFilter;

	m_pPiraq_BaseAddress	= NULL;
	m_pPLX								= NULL;
	m_pControl						= NULL;
	m_pHpib								= NULL;
	m_pFirFilter					= NULL;
}





/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::SetErrorString(char* pErrorString)
//
// DESCRIPTION: Sets the error string 
//
// INPUT PARAMETER(s): 
// char* pErrorString - Stored error string from last error
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
void PIRAQ::SetErrorString(char* pErrorString)
{
	strcpy(m_cErrorString, pErrorString);
}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::GetErrorString(char* pErrorString)
//
// DESCRIPTION: Returns the error string to the host application 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): 
// char* pErrorString - Stored error string from last error
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
void PIRAQ::GetErrorString(char* pErrorString)
{
	strcpy(pErrorString, m_cErrorString);
}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::Init( unsigned short shVendorID ,  unsigned short shDeviceID )
//
// DESCRIPTION: Initializes the driver by creating classes to handle the different 
// sections of the Piraq board. 
//
// PLX Class handles the PLX PCI interface chip
// CONTROL Class handles the Status Registers and the Timers
// HPIB Class handles writing code to the DSP chip
// FIRFILTER Class handles the Fir Filters
//
// m_pPiraq_BaseAddress is the base address of the Piraq Board's registers, timers, etc. 
// m_pPLX_BaseAddress is the base address of the PLX chip and it's EPROM
// m_pPCI_Memory is the base address of the CommonBufferMemory used by both the host
// windows application and the Piraq card to pass data back and forth.
// The size of this memory is set in the NT registry and is allocated at boot up.
//
// PIRAQ_MAILBOX2 is written w/m_pCommonBufferAddressOn8MegBounderyUser, base of 
// shared memory; this value is also returned by member function GetBuffer(). 11-01-02 mp. 
//
// Calculations related to memory allocation were built around the assumption of a single board 
// in the system. First pass at multi-board function operational -- return and structure more 
// cleanly. 02-18-03 mp. 
// 
// INPUT PARAMETER(s): 
// unsigned short shVendorID - ID used to identify the board
// unsigned short shDeviceID - ID used to identify the board
//
// OUTPUT PARAMETER(s): None
//
// RETURN: error code - 0=OK -1=Error         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY: Milan Pipersky 02-18-03
//
/////////////////////////////////////////////////////////////////////////////////////////
int PIRAQ::Init( unsigned short shVendorID ,  unsigned short shDeviceID )
{
	// Pointers to classes
	m_pPLX = NULL;
	m_pControl = NULL;
	m_pHpib = NULL;
	m_pFirFilter = NULL;
	static U32 DeviceCount = 0;

	// Pointers to memory
	m_pPiraq_BaseAddress = NULL;
	m_pPLX_BaseAddress = NULL;
	m_pPCI_Memory = NULL;

	// Create PLX class to gain access to PLX board
	m_pPLX = new PLX();
	if(m_pPLX == NULL)
	{
		SetErrorString("Could not instantiate class for PLX board.");
		return(-1);
	}

	// Initialize PLX class
	lReturnValue = ((PLX*)m_pPLX)->InitPLX(shVendorID , shDeviceID );
	if(lReturnValue != 0)
	{
		SetErrorString("Could not initialize PLX class.");
		return(-1);
	}

	// Get m_pPiraq_BaseAddress which address the board itself and it's registers
	m_pPiraq_BaseAddress = ((PLX*)m_pPLX)->GetPiraqBaseAddress();
	if(m_pPiraq_BaseAddress == NULL)
	{
		SetErrorString("Could not get Piraq base address.");
		return(-1);
	}

	// Get m_pPLX_BaseAddress which address the EPROM
	m_pPLX_BaseAddress = ((PLX*)m_pPLX)->GetPLXBaseAddress();
	if(m_pPLX_BaseAddress == NULL)
	{
		SetErrorString("Could not get PLX base address.");
		return(-1);
	}

	// Get m_pPC_BaseAddress which address ??
	m_pPC_BaseAddress	= ((PLX*)m_pPLX)->GetPCBaseAddress();
	if(m_pPC_BaseAddress == NULL)
	{
		SetErrorString("Could not get PC base address.");
		return(-1);
	}

	// Get m_pPCI_Memory which address the CommonBufferMemory that is shared
	// between the Host application and the driver to pass data back and forth
	// PCI Memory  Structure
	//typedef struct _PCI_MEMORY
	//{
	//    U32 UserAddr;
	//    U32 PhysicalAddr;
	//    U32 Size;
	//} PCI_MEMORY;
	m_pPCI_Memory	= ((PLX*)m_pPLX)->GetPCIMemoryBaseAddress();
	printf("m_pPCI_Memory->UserAddr = 0x%x m_pPCI_Memory->PhysicalAddr = 0x%x m_pPCI_Memory->Size = 0x%x\n", ((PCI_MEMORY*)m_pPCI_Memory)->UserAddr, ((PCI_MEMORY*)m_pPCI_Memory)->PhysicalAddr, ((PCI_MEMORY*)m_pPCI_Memory)->Size); 

	if(m_pPCI_Memory == NULL)
	{
		SetErrorString("Could not get PCI Memory base address.");
		return(-1);
	}

	// Check size of CommonBufferMemory to make sure it allocated 8 megabytes
	// The size is set in the NT registry
	// The PLX PCI chip request allocation of this space at boot up.
	//	if(((PCI_MEMORY*)m_pPCI_Memory)->Size != 0x1000000) // 16MB total
	//	if(((PCI_MEMORY*)m_pPCI_Memory)->Size != 0x2000000) // 32MB total: preliminary success
	if(((PCI_MEMORY*)m_pPCI_Memory)->Size != 0x3000000) // 48MB total: preliminary success
	{
		char buf[255];
		sprintf(buf,"Set up registry for commonbuffersize! Size is set at %8X",((PCI_MEMORY*)m_pPCI_Memory)->Size);
		//SetErrorString("Set HLM\\System\\CurrentControlSet\\Services\\PCI9054\\CommonBufferSize=0x10000000.");
		SetErrorString(buf);
		return(-1);
	}

	DeviceCount++;	// no error return -- increment device count -- and add 8MB to 
	// m_pCommonBufferAddressOn8MegBounderyPhysical for each successful instantiation. 
	// Due to factors of the PLX chip, we had to allocate twice as much memory (16Megabytes) 
	// and then shift the address to use 8 Megabyes starting at and even 8 Megabyte boundery. 
	//!!!	m_pCommonBufferAddressOn8MegBounderyPhysical	= (unsigned long *)((char*)(((PCI_MEMORY*)m_pPCI_Memory)->PhysicalAddr & 0xff800000) + 0x800000);
	m_pCommonBufferAddressOn8MegBounderyPhysical	= (unsigned long *)((char*)(((PCI_MEMORY*)m_pPCI_Memory)->PhysicalAddr & 0xff800000) + (0x800000*DeviceCount));
	printf("DeviceCount = 0x%x\n", DeviceCount); 
	printf("m_pCommonBufferAddressOn8MegBounderyPhysical = 0x%x\n", m_pCommonBufferAddressOn8MegBounderyPhysical); 
	unsigned long lDifference = (unsigned long) m_pCommonBufferAddressOn8MegBounderyPhysical - (unsigned long) ((PCI_MEMORY*)m_pPCI_Memory)->PhysicalAddr;
	printf("lDifference = 0x%x\n", lDifference); 
	m_pCommonBufferAddressOn8MegBounderyUser	= (unsigned long *)((char*)((PCI_MEMORY*)m_pPCI_Memory)->UserAddr + lDifference);
	printf("m_pCommonBufferAddressOn8MegBounderyUser = 0x%x\n", m_pCommonBufferAddressOn8MegBounderyUser); 
	m_lCommonBufferLength = ((PCI_MEMORY*)m_pPCI_Memory)->Size/4/2;
	//m_lCommonBufferLength = 0x200000;
	printf("m_lCommonBufferLength = 0x%x\n", m_lCommonBufferLength); 


	// Set the pointer to the PC system memory "CommonBufferMemory"  into PLX chip
	// This tells the PLX PCI chip to route DSP calls to memory to this address
	unsigned long* pPLX_DMPBAM = (unsigned long *)((unsigned char*)m_pPLX_BaseAddress + DMPBAM);
	*pPLX_DMPBAM = (unsigned long)( (*pPLX_DMPBAM & 0x0000FFFF)  | ((unsigned long)m_pCommonBufferAddressOn8MegBounderyPhysical & 0xFFFF0000));
	printf("*pPLX_DMPBAM = 0x%x\n", *pPLX_DMPBAM); 

	// Set the pointer to  "CommonBufferMemory"in PLX chip mailbox 2 for use by DSP program
	unsigned long * pPLX_PIRAQ_MAILBOX2 = (unsigned long *)((unsigned char*)m_pPLX_BaseAddress + PIRAQ_MAILBOX2);
	*pPLX_PIRAQ_MAILBOX2 = (unsigned long)(((unsigned long)m_pCommonBufferAddressOn8MegBounderyPhysical & 0xFFFF0000));

	// Set the length in bytes of the "CommonBufferMemory"
	unsigned long* pPLX_DMRR = (unsigned long *)((unsigned char*)m_pPLX_BaseAddress + DMRR);
	*pPLX_DMRR = 0xff800000;

	// Set the address that the DSP will use to write to the "CommonBufferMemory"
	// This remaps a constant address for the DSP to a changing address in PC memory
	unsigned long* pPLX_DMLBAM = (unsigned long *)((unsigned char*)m_pPLX_BaseAddress + DMLBAM);
	*pPLX_DMLBAM = 0x0;

	m_pSemaphores = (unsigned long *)((unsigned char*)m_pCommonBufferAddressOn8MegBounderyUser + SEMAPHOREOFFSET);

	// Create classes
	m_pControl						= new CONTROL(m_pPiraq_BaseAddress);
	m_pHpib							= new HPIB(m_pPiraq_BaseAddress);
	m_pFirFilter					= new FIRFILTER(m_pPiraq_BaseAddress);

	// Set up the timer pointer
	timer = ((CONTROL *)m_pControl)->GetTimer();

	// Set up the FIRCH pointers
	FIRCH1_I = GetFilter()->GetFIRCH1_I();
	FIRCH1_Q = GetFilter()->GetFIRCH1_Q();
	FIRCH2_I = GetFilter()->GetFIRCH2_I();
	FIRCH2_Q = GetFilter()->GetFIRCH2_Q();

	// Set default PIRAQ Data Mode in MAILBOX5: SEND_COMBINED, execute the dynamic range extension algorithm 
	unsigned long * pPLX_PIRAQ_MAILBOX5 = (unsigned long *)((unsigned char*)m_pPLX_BaseAddress + PIRAQ_MAILBOX5);
	*pPLX_PIRAQ_MAILBOX5 = (unsigned long)SEND_COMBINED;

	return 0;
}





/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ:: GetCommonBufferMemoryPointer()
//
// DESCRIPTION: Returns the virtual address for the common memory used by both
// the PLX PCI chip and the Windows host application. This is how data is passed
// back and forth from the DSP and LAPXM 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): None
//
// RETURN: m_pPCI_Memory->UserAddr - Virtual address of memory space         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
void PIRAQ::GetCommonBufferMemoryPointer(unsigned long **pCommonMemoryAddress, unsigned long *plLength)
{
	// Pass back the address to the common buffer memory.
	// This address starts on an even 8 Megabyte boundry.
	// To do this, we allocated 16 megabytes of space and chopped out 
	// the 8 megabytes that started on this boundery
	*pCommonMemoryAddress = (unsigned long*)m_pCommonBufferAddressOn8MegBounderyUser;

	// Pass back the length to only 8 megabytes in 32 bit words 
	*plLength = m_lCommonBufferLength;

}




// Used for Testing ***************************


/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::ToggleLED()
//
// DESCRIPTION: Calls on the CONTROL class to toggle the LED 
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
void PIRAQ::ToggleLED()
{	
	((CONTROL*)m_pControl)->ToggleLED();
}





/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::ReadEPROM(unsigned int * pEPROM)
//
// DESCRIPTION: Returns the values set in the EPROM. Note that some of them have
// been changed in the Init() 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): None
// unsigned int * pEPROM - Array passed in by host to get the values of the EPROM
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY: Milan Pipersky 4-19-04 Increase elements transferred from 64 to 128:
//                                          callers must allocate sufficient buffer to accomodate. 
//
//
/////////////////////////////////////////////////////////////////////////////////////////
void PIRAQ::ReadEPROM(unsigned int * pEPROM)
{	
	for(int i=0; i< 128; i++)
	{
		pEPROM[i] = m_pPLX_BaseAddress[i];
	}
}





/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::PCMemoryTest(unsigned int StartValue)
//
// DESCRIPTION: Writes a pattern to the 8 Megabytes of memory shared by the Piraq and the
// host application. It then reads the pattern back to make sure it was written correctly.
// The host application can then read it back for a second test. 
//
// INPUT PARAMETER(s): 
// unsigned int StartValue -- test begin data value, incremented for each location. 
//
// OUTPUT PARAMETER(s): None
//
// RETURN: error code - 0=OK -1=Error         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY: Milan Pipersky 2-14-03  StartValue set by caller for diagnostic purposes 
//                                           and to distinguish cards for shared memory tests. 
//
/////////////////////////////////////////////////////////////////////////////////////////
int PIRAQ::PCMemoryTest(unsigned int StartValue)
{	
	// Write and read back a pattern from the common memory on the PC that is
	// used for passing data from the DSP to the Windows application such as Lapxm.
	// This memory is allocated on boot up using the following registry entry
	// to determine the size. Currently it is set to 8 Megabytes which is 0x7FFFFF

	// Get Address to Space
	unsigned long * pCommonBufferMemory = m_pCommonBufferAddressOn8MegBounderyUser;

	// Get the size in 32bit words
	unsigned long lSize = m_lCommonBufferLength;

	// Write Pattern
	for(unsigned long i = 0; i< lSize; i++) // 256 heights * 4096 points * 2 (I&Q) - 4byte floats
	{
		//		*pCommonBufferMemory = (0x0001 + i);
		*pCommonBufferMemory = (StartValue + 4*i);
		pCommonBufferMemory = pCommonBufferMemory + 1;
	}

	// Check Patten
	pCommonBufferMemory = m_pCommonBufferAddressOn8MegBounderyUser;
	for(unsigned long j = 0; j< lSize; j++)
	{

		//		if(*pCommonBufferMemory != (0x0001 + j))
		if(*pCommonBufferMemory != (StartValue + 4*j))
		{
			// If pattern fails, return error
			SetErrorString("Failed PC Memory Test");
			return (0);
		}

		pCommonBufferMemory = pCommonBufferMemory + 1;

	}


	// If pattern passes, return OK.
	// Windows application shoud then test pattern itself
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::ReturnTimerValues(unsigned int* piCountsPerDelay,	unsigned int* piCountsForAllGates,
//																unsigned int* piCountsPerGatePulse,unsigned short* piSpare)
//
// DESCRIPTION: Returns to the host the current timer values 
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
void PIRAQ::ReturnTimerValues(unsigned int* piCountsPerDelay,	unsigned int* piCountsForAllGates,
							  unsigned int* piCountsPerGatePulse,unsigned short* piStatus0,unsigned short* piStatus1)
{
	((CONTROL*)m_pControl)->ReturnTimerValues(piCountsPerDelay,	piCountsForAllGates, piCountsPerGatePulse,piStatus0,piStatus1);
}






/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::ReturnFilterValues(unsigned short	*pFIR, unsigned int* piCount)
//
// DESCRIPTION: Returns to the host application the current Fir Filter values.
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): 
// unsigned short	*pFIR - An array to hold the Fir Filter values
// unsigned int* piCount - The number of values
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
void PIRAQ::ReturnFilterValues(unsigned short	*pFIR, unsigned int* piCount)
{
	((FIRFILTER*)m_pFirFilter)->ReturnFilterValues(pFIR, piCount);
}








/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::ResetPiraq()
//
// DESCRIPTION: Calls on the CONTROL class to put the DSP in a known state 
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
void PIRAQ::ResetPiraq()
{	
	((CONTROL*)m_pControl)->ResetPiraqBoard();
}





/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::LoadDspCode(char* pFilePathAndName)
//
// DESCRIPTION: Loads COFF type 2 code into the DSP 
//
// INPUT PARAMETER(s): 
// char* pFilePathAndName - Full path and file name of COFF type 2 code to load into the DSP
//
// OUTPUT PARAMETER(s): None
//
// RETURN: error code - 0=OK -1=Error         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
int PIRAQ::LoadDspCode(char* pFilePathAndName)
{	
	char cErrorString[256];
	lReturnValue = ((HPIB*)m_pHpib)->coffload(pFilePathAndName, cErrorString);
	if(lReturnValue != 0)
	{
		if(lReturnValue == -1)
		{
			SetErrorString(cErrorString);
			return (-1);
		}
	}

	return (0);
}






/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::StartDsp()
//
// DESCRIPTION: Calls on the HPIB class to start the DSP 
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
void PIRAQ::StartDsp()
{	
	((HPIB*)m_pHpib)->StartDsp();

}





/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::LoadParameters(float fFrequencyHz, long lTransmitPulseWidthNs,
//													 long lPreTRNs, long lTimeDelayToFirstGateNs,
//													 long lGateSpacingNs, long lNumRangeGates,
//													 long lNumCodeBits, long lNumCoherentIntegrations,
//													 long lNumPointsInSpectrum,int iFilterType)
//
// DESCRIPTION: Receives the parameters from the host application and used them to
// set up the timers, the filters, and the DSP. 
//
// INPUT PARAMETER(s): 
// float fFrequencyHz - Piraq Clock Frequency in Hz
// long lTransmitPulseWidthNs - Width of the Transmit pulse in nanoseconds
// long lPreTRNs - Time from start of Sync pulse to start of Transmit pulse in nanoseconds
// long lTimeDelayToFirstGateNs - Time from end of Transmit pulse to start of first sampling gate in nanoseconds
// long lGateSpacingNs - Time from one sampling gate to the next in nanoseconds
// long lNumRangeGates - Number of sampling gates (Nhts)
// long lNumCodeBits - Number of code bits used in coding (NCode)
// long lNumCoherentIntegrations - Number of coherent integrations used (NCI)
// long lNumPointsInSpectrum - Number of FFTS used (Npts)
// int	iFilterType - 0=Gausian - 1=Half Bandwidth
// OUTPUT PARAMETER(s): None
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
void PIRAQ::LoadParameters(float fFrequencyHz, long lTransmitPulseWidthNs,
						   long lPreTRNs, long lTimeDelayToFirstGateNs,
						   long lGateSpacingNs, long lNumRangeGates,
						   long lNumCodeBits, long lNumCoherentIntegrations,
						   long lNumPointsInSpectrum, long lNumReceivers,
						   int iTimerDivide, int iFilterType, long lFlip)
{	

	//	Transmit Pulse Width must be at least 300ns
	if(lTransmitPulseWidthNs < 300)
	{
		// Defualt to 300
		lTransmitPulseWidthNs = 300;
	}

	// Convert lTransmitPulseWidthNs & iGateSpacingClockCycles into the correct multiple of the clock
	int iGateSpacingClockCycles = (int)( lGateSpacingNs * 1e-9 * fFrequencyHz + .5 );
	int iTransmitPulseWidthClockCycles = (int)( lTransmitPulseWidthNs * 1e-9 * fFrequencyHz + .5 );

	// Calculate new Pulse Width and GateSpacing
	lGateSpacingNs				= (long)(iGateSpacingClockCycles * (1e9/fFrequencyHz));
	lTransmitPulseWidthNs = (long)(iTransmitPulseWidthClockCycles * (1e9/fFrequencyHz));

	if ( lTransmitPulseWidthNs < 1000 /*ns*/) 
	{
		// GateSpacing and TX must be an even number of clock cycles due to the fact that
		// the GLEN counter holds only one half of the GateSpacing. Thus GateSpacing is forced
		// to be an even multiple of 2.
		if(iGateSpacingClockCycles & 0x1)
		{
			iGateSpacingClockCycles = iGateSpacingClockCycles + (iGateSpacingClockCycles % 2);
		}
		lGateSpacingNs = (long)(iGateSpacingClockCycles * (1e9/fFrequencyHz));

		if(iTransmitPulseWidthClockCycles & 0x1)
		{
			iTransmitPulseWidthClockCycles = iTransmitPulseWidthClockCycles + (iTransmitPulseWidthClockCycles % 2);
		}		
		lTransmitPulseWidthNs = (long)(iTransmitPulseWidthClockCycles * (1e9/fFrequencyHz));

	}
	else if ( (lTransmitPulseWidthNs >= 1000 /*ns*/) &&  (lTransmitPulseWidthNs < 2000 /*ns*/) )
	{
		// GateSpacing and TX must be an even multiple of 4 clock cycles due to decimation or 2 
		// and the GLEN counter divides the count in half
		if(iGateSpacingClockCycles % 4)
		{
			iGateSpacingClockCycles = iGateSpacingClockCycles + (iGateSpacingClockCycles % 4);
		}
		lGateSpacingNs = (long)(iGateSpacingClockCycles * (1e9/fFrequencyHz));

		if(iTransmitPulseWidthClockCycles % 4)
		{
			iTransmitPulseWidthClockCycles = iTransmitPulseWidthClockCycles + (iTransmitPulseWidthClockCycles % 4);
		}		
		lTransmitPulseWidthNs = (long)(iTransmitPulseWidthClockCycles * (1e9/fFrequencyHz));
	}
	else if ( lTransmitPulseWidthNs >= 2000 /*ns*/ )
	{
		// GateSpacing and TX must be an even multiple of 4 clock cycles due to decimation or 4 
		// and the GLEN counter divides the count in half
		if(iGateSpacingClockCycles % 8)
		{
			iGateSpacingClockCycles = iGateSpacingClockCycles + (8 - (iGateSpacingClockCycles % 8));
		}
		lGateSpacingNs = (long)(iGateSpacingClockCycles * (1e9/fFrequencyHz));

		if(iTransmitPulseWidthClockCycles % 8)
		{
			iTransmitPulseWidthClockCycles = iTransmitPulseWidthClockCycles + (8 - (iTransmitPulseWidthClockCycles % 8));
		}
		lTransmitPulseWidthNs = (long)(iTransmitPulseWidthClockCycles * (1e9/fFrequencyHz));

	}

	// Only divide by 8 supported at this time
	if(iTimerDivide != 8) 
	{
		iTimerDivide = 8;
	}

	// Set up the timers
	((CONTROL*)m_pControl)->SetUpTimers(fFrequencyHz, lTransmitPulseWidthNs, lPreTRNs,
		lTimeDelayToFirstGateNs, lGateSpacingNs, lNumRangeGates,
		iTimerDivide, iGateSpacingClockCycles,lNumCodeBits);


	// Only Gaussian filter supported at this time
	if(iFilterType != 0) 
	{
		iFilterType = 0;
	}

	// Set up the Fir Filters
	if(iFilterType == 1)
	{
		// Half Band Width Filter
		((FIRFILTER*)m_pFirFilter)->FIR_HalfBandWidth(fFrequencyHz, lTransmitPulseWidthNs, lNumCodeBits);
	}
	else if(iFilterType == 2)
	{
		// Box Car Filter
		((FIRFILTER*)m_pFirFilter)->FIR_BoxCar(fFrequencyHz, lTransmitPulseWidthNs, lNumCodeBits);
	}
	else
	{
		// Defualt
		// Gaussian Filter
		((FIRFILTER*)m_pFirFilter)->Gaussian(fFrequencyHz, lTransmitPulseWidthNs, lNumCodeBits);
	}
	// Write parameters to the CommonBufferMemory so that the DSP can read them in

	// Get Address to Space
	volatile int * pParameters;
	pParameters = (volatile int *)((char*)m_pCommonBufferAddressOn8MegBounderyUser + PARAMETEROFFSET);
	pParameters[NUMCODEBITS]							= lNumCodeBits;
	pParameters[NUMCOHERENTINTEGRATIONS]	= lNumCoherentIntegrations;
	pParameters[NUMRANGEGATES]						= lNumRangeGates;
	pParameters[NUMPOINTSINSPECTRUM]			= lNumPointsInSpectrum;
	pParameters[NUMRECEIVERS]							= lNumReceivers;
	pParameters[FLIP]											= lFlip;


	((CONTROL*)m_pControl)->StartTimers();
}













/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::LoadFIRParameters(float sp_fFrequencyHz, long sp_lTransmitPulseWidthNs,
//													 long sp_lNumCodeBits)
//
// DESCRIPTION: Receives the parameters from the host application and used them to
// set up the timers, the filters, and the DSP. 
//
// INPUT PARAMETER(s): 
// float sp_fFrequencyHz - Piraq Clock Frequency in Hz
// long sp_lTransmitPulseWidthNs - Width of the Transmit pulse in nanoseconds
// long sp_lNumCodeBits - Number of code bits used in coding (NCode)
// OUTPUT PARAMETER(s): None
//
// RETURN: none         
//
// WRITTEN BY: Milan Pipersky 11-21-02. Temporary member function until PIRAQ::LoadParameters fully implemented. 
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////

void PIRAQ::LoadFIRParameters(float sp_fFrequencyHz, long sp_lTransmitPulseWidthNs, long sp_lNumCodeBits)
{	

	// Defualt
	// Gaussian Filter
	((FIRFILTER*)m_pFirFilter)->Gaussian(sp_fFrequencyHz, sp_lTransmitPulseWidthNs, sp_lNumCodeBits);

	//?que es?	pParameters[FLIP]											= lFlip;

}










/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::StopDsp()
//
// DESCRIPTION: Calls on the Control class to perform a Software Reset  
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
void PIRAQ::StopDsp()
{	
	((CONTROL*)m_pControl)->StopDsp();
}


/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ:: SemaSet(int iSemaphore)
//
// DESCRIPTION: Sets the value of the semaphore in local PC memory for the DSP to read.
// Interupts the DSP so that it will read the semaphore and move on.
//
// INPUT PARAMETER(s): 
// int iSemaphore - Value of semaphore to set into memory
//
// OUTPUT PARAMETER(s): None
//
// RETURN: long - not used at this time         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
long PIRAQ::SemaSet(int iSemaphore)
{	
	// Set semaphore value 
	m_pSemaphores[0] = iSemaphore;

	// Interupt DSP so that it will know a semaphore has been set.
	((CONTROL*)m_pControl)->InteruptDSP();

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::SemaWait(int iSemaphore, int iTimeOutInSeconds)
//
// DESCRIPTION: Waits for the DSP to set a semaphore to indicate the code can move on.
//
// INPUT PARAMETER(s): 
// int iSemaphore - Value of semaphore we are looking for
// int iTimeOutInSeconds - Time in seconds to wait for semaphore
//
// OUTPUT PARAMETER(s): None
//
// RETURN: long - 0 if semaphore was received in time. -1 if not.         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
long  PIRAQ::SemaWait(int iSemaphore, int iTimeOutInMiliSeconds)
{	

	int iCount = 0;
	int iMaxCount = (int)ceil((iTimeOutInMiliSeconds)/(float)10);

	// Read first
	while((m_pSemaphores[0] != (unsigned int)iSemaphore) && (iCount <iMaxCount) )
	{
		// If value has not been set yet, wait 1/100th of a second and check again
		Sleep(10);
		iCount++;
	}

	// Did we time out
	if( iCount >= iMaxCount)
	{
		return -1;
	}

	return 0;
}



/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::SetPMACAntennaDPRAMAddress(unsigned int * PMACAntennaDPRAMAddress) 
//
// DESCRIPTION: Set PMAC Dual-Port RAM base address in PIRAQ Mailbox 4.
//
// INPUT PARAMETER(s): 
// unsigned int * PMACAntennaDPRAMAddress
//
// OUTPUT PARAMETER(s): None
//
// RETURN: none.         
//
// WRITTEN BY: Milan Pipersky 4-23-04
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////

void  PIRAQ::SetPMACAntennaDPRAMAddress(unsigned int * PMACAntennaDPRAMAddress)
{	

	// Compute pointer to  PIRAQ mailbox 4; put passed address there 
	unsigned long * pPLX_PIRAQ_MAILBOX4 = (unsigned long *)((unsigned char*)(GetRegisterBase()) + PIRAQ_MAILBOX4);
	*pPLX_PIRAQ_MAILBOX4 = (unsigned long)PMACAntennaDPRAMAddress;

}



/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::GetPMACAntennaDPRAMAddress(unsigned int * PMACAntennaDPRAMAddress) 
//
// DESCRIPTION: Get PMAC Dual-Port RAM base address from PIRAQ Mailbox 4.
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): None
// unsigned int * PMACAntennaDPRAMAddress
//
// RETURN: none.         
//
// WRITTEN BY: Milan Pipersky 4-23-04
//
// LAST MODIFIED BY: Milan Pipersky 3-10-06
//
/////////////////////////////////////////////////////////////////////////////////////////

unsigned int * PIRAQ::GetPMACAntennaDPRAMAddress()
{	

	// Compute pointer to  PIRAQ mailbox 4; get PMAC DPRAM address stored there for PIRAQ use
	unsigned long * pPLX_PIRAQ_MAILBOX4 = (unsigned long *)((unsigned char*)(GetRegisterBase()) + PIRAQ_MAILBOX4);
	unsigned int * PIRAQ_PMACDPRAM = (unsigned int *)*pPLX_PIRAQ_MAILBOX4; 
	return(PIRAQ_PMACDPRAM); 

}


/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PIRAQ::SetCP2PIRAQTestAction(unsigned short PIRAQTestAction) 
//
// DESCRIPTION: Set PIRAQ channel mode in PIRAQ Mailbox 5. 
// 3 modes of data transfer from PIRAQ to host: first 2 are diagnostic
//			SEND_CHA				// send CHA
//			SEND_CHB				// send CHB
//			SEND_COMBINED			// execute dynamic-range extension algorithm; send resulting  
//									   combined data
//			//	PIRAQ DATA TEST-SINUSOID ADJUSTMENTS: step frequency or amplitude
//			INCREMENT_TEST_SINUSIOD_COARSE
//			INCREMENT_TEST_SINUSIOD_FINE
//			DECREMENT_TEST_SINUSIOD_COARSE
//			DECREMENT_TEST_SINUSIOD_FINE
//			INCREMENT_TEST_AMPLITUDE_COARSE
//			INCREMENT_TEST_AMPLITUDE_FINE
//			DECREMENT_TEST_AMPLITUDE_COARSE
//			DECREMENT_TEST_AMPLITUDE_FINE
//			Definitions are in proto.h also for time being. 
//
// INPUT PARAMETER(s): 
// unsigned short PIRAQTestAction
//
// OUTPUT PARAMETER(s): None
//
// RETURN: none.         
//
// WRITTEN BY: Milan Pipersky 2-23-06
//
// LAST MODIFIED BY: Milan Pipersky 3-10-06
//
/////////////////////////////////////////////////////////////////////////////////////////

void  PIRAQ::SetCP2PIRAQTestAction(unsigned short PIRAQTestAction)
{	
	// Compute pointer to  PIRAQ mailbox 5; put PIRAQ test parameter there 
	unsigned long * pPLX_PIRAQ_MAILBOX5 = (unsigned long *)((unsigned char*)(GetRegisterBase()) + PIRAQ_MAILBOX5);
	*pPLX_PIRAQ_MAILBOX5 = (unsigned long)PIRAQTestAction;

}






