/////////////////////////////////////////////////////////////////////////////////////////
// FILE NAME: HPIB.h
//
// DESCRIPTION: HPIB Class implementation. This class handles the HPIB interface
// to the DSP.
//
/////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#define LO      shrt[0]
#define HI      shrt[1]
#define DSPINT  0x0001
#define HINT  0x0002
#define STRMAX 66000
#define HPIbuffsize 2000

#ifdef	LITTLE_ENDIAN
 #define		HWOB	0x0001
	// first half word is the most significant half
#else
 #define		HWOB	0x0000
	// first half word is the least significant half
#endif
#define export __declspec(dllexport)

class export HPIB
 {
  private:
		unsigned long			lReturnValue;
		unsigned int			*m_pBaseAddress;
		unsigned short		*m_pHPICLO, *m_pHPICHI;		// hpi control registers LO and HI
		unsigned short		*m_pHPIALO, *m_pHPIAHI;		// hpi Address registers LO and HI
		unsigned short		*m_pHPIDLO, *m_pHPIDHI;		// hpi Data Registers LO and HI
		unsigned short		*m_pHPIDLA, *m_pHPIDHA;		// set up autoincrement registers
		int								m_FirstHpi;

		typedef union   
		{
			unsigned int        lohi;
			unsigned short  shrt[2];
		} SHORT2LONG;

		struct InputBufferStructure
		{
			unsigned short data[STRMAX];
		};

		struct FileHeaderStructure 
		{
			 unsigned short version;
			 unsigned short sections;
			 long stamp;
			 long symptr;
			 long symbols;
			 unsigned short optbytes;
			 unsigned short flags;
			 unsigned short magic;
		};

		struct SectionHeaderStructure 
		{
			 char name[8];
			 long phyaddr;
			 long virtaddr;
			 long sectsize;
			 long rawdataptr;
			 long relocationptr;
			 long linnumptr;
			 unsigned long relocationents;
			 unsigned long linnums;
			 unsigned long flags;
			 short resv;
			 char page;
		};

  public:
		HPIB(unsigned int *pBaseAddress);
		~HPIB();
		void StartDsp();
		long  coffload( char *cTargetfile, char* cErrorString);
		long Download (struct InputBufferStructure *aData, unsigned long lDestinationAddressr, unsigned long lLength);
		unsigned long ReadLocation( unsigned int iAddress );
		void WriteLocation( unsigned int iAddress, unsigned int iData );

};// end of class HPIB definition

