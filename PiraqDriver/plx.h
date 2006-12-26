/////////////////////////////////////////////////////////////////////////////////////////
// FILE NAME: Plx.h
//
// DESCRIPTION: PLX Class implementation. This class handles the PLX PCI chip
//
/////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "stdafx.h"

// Define a large value for a signal to the driver
#define FIND_AMOUNT_MATCHED              80001
#define export __declspec(dllexport)

class export PLX 
{
	private:
		unsigned long lReturnVal;

		VIRTUAL_ADDRESSES	m_pVirtualAddresses;
		HANDLE						m_pPLX_Handle;
		DEVICE_LOCATION		m_pPLX_Device;				// Device Location Information 
		PCI_MEMORY				m_pPCI_Memory;				// PC System Memory
		unsigned int *		m_pPiraq_BaseAddress; // Registers, Timers, Filters, HPIB
		unsigned long *		m_pPLX_BaseAddress;		// EPROM
		unsigned long *		m_pPC_BaseAddress;		// ??

public:
	PLX(void);
	~PLX(void);

	int InitPLX(	unsigned short shVendorID, 
				unsigned short shDeviceID,
				unsigned short shBus      = (unsigned short) 0xFFFFFFFF,
				unsigned short shSlot     = (unsigned short) 0xFFFFFFFF,
				char *cSN="\0"  );


	unsigned int* GetPiraqBaseAddress(void);
	unsigned long* GetPLXBaseAddress(void);
	unsigned long* GetPCBaseAddress(void);
	PCI_MEMORY* GetPCIMemoryBaseAddress(void);

}; // end of class PLX

