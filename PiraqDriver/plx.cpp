/////////////////////////////////////////////////////////////////////////////////////////
// FILE NAME: Plx.cpp
//
// DESCRIPTION: PLX Class implementation. This class handles the PLX PCI chip
//
/////////////////////////////////////////////////////////////////////////////////////////
#include "PLX.h"


/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PLX::PLX()
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
PLX::PLX()
{
}






/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PLX::~PLX(void)
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
PLX::~PLX(void)
{
	if (m_pPLX_Handle!=(HANDLE)0xffffffffff)
	{
		lReturnVal = PlxPciDeviceClose( m_pPLX_Handle );
	}
}









/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PLX::InitPLX(  unsigned short shVendorID, unsigned short shDeviceID, unsigned short shBus, unsigned short shSlot, char *cSN )
//
// DESCRIPTION: Searches for the PLX PCI chip that is identified by the IDs.
// Opens the board.
// Obtains addresses to the differnt parts such as - EPROM, Timers, Filters, Registers and Common Memory 
//
// INPUT PARAMETER(s): 
// unsigned short shVendorID - ID that identifies the board
// unsigned short shDeviceID - ID that identifies the board
//
// OUTPUT PARAMETER(s): 
// unsigned short shBus - The bus the board was found on
// unsigned short shSlot - The slot it was in
// char *cSN 
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY: Milan Pipersky 02-18-03
//
/////////////////////////////////////////////////////////////////////////////////////////
PLX::InitPLX(  unsigned short shVendorID, unsigned short shDeviceID, unsigned short shBus, unsigned short shSlot, char *cSN )
{
	m_pPLX_Handle=(HANDLE)0xffffffffff;
	m_pPLX_Device.DeviceId=shDeviceID;
	m_pPLX_Device.VendorId=shVendorID;
	m_pPLX_Device.BusNumber=0xff;
	m_pPLX_Device.SlotNumber=0xff;
	m_pPLX_Device.SerialNumber[0]='\0';
	static U32 DeviceNum = 0;

	// Search for baord
	lReturnVal = PlxPciDeviceFind( &m_pPLX_Device, &DeviceNum);
	if (lReturnVal != ApiSuccess) 
	{
		return -1;
	}
printf("m_pPLX_Device.SlotNumber = 0x%x\n", m_pPLX_Device.SlotNumber); 	
printf("DeviceNum = 0x%x\n", DeviceNum); 	
//!!!as it was:	DeviceNum=1;
DeviceNum++; // gets a distinct value on each PLX instantiation

	// Open board
	lReturnVal = PlxPciDeviceOpen( &m_pPLX_Device, &m_pPLX_Handle );
	if (lReturnVal!=ApiSuccess)
	{
		return -1;
	}

	// Get address to common memory used to pass data between the DSP and the host
	lReturnVal = PlxPciCommonBufferGet(m_pPLX_Handle,&m_pPCI_Memory);
	if (lReturnVal!=ApiSuccess)
	{
		return -1;
	}

	// Get the base addresses of the EPROM, Regesters, Filters and Timers 
	lReturnVal = PlxPciBaseAddressesGet(m_pPLX_Handle,&m_pVirtualAddresses);
	if (lReturnVal!=ApiSuccess)
	{
		return -1;
	}

	// Base adddress of PLX PCI chip and the EPROM
	m_pPLX_BaseAddress		= ((unsigned long* )m_pVirtualAddresses.Va0);

	// Base address of the Registers, Timers and Filters
	m_pPiraq_BaseAddress	= ((unsigned int*)m_pVirtualAddresses.Va2);

	// Unknown base address
	m_pPC_BaseAddress			= ((unsigned long*)m_pVirtualAddresses.Va3);

	return 0;
}








/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PLX::GetPiraqBaseAddress()
//
// DESCRIPTION: Returns address to Timers, Registers, Filters Etc. 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): None
//
// RETURN: Address         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
unsigned int* PLX::GetPiraqBaseAddress()
{
	return(m_pPiraq_BaseAddress);
}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PLX::GetPLXBaseAddress(void)
//
// DESCRIPTION: Returns address to the PLX PCI chip and EPROM 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): None
//
// RETURN: Address         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
unsigned long* PLX::GetPLXBaseAddress(void)
{
	return(m_pPLX_BaseAddress);
}








/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PLX::GetPCBaseAddress()
//
// DESCRIPTION: Returns the address to ???? 
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): None
//
// RETURN: Address         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
unsigned long* PLX::GetPCBaseAddress()
{
	return(m_pPC_BaseAddress);
}









/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: PLX::GetPCIMemoryBaseAddress()
//
// DESCRIPTION: Returns the physical and virtual base addresses to the common memory along
// with it's size. The common memory is allocated at boot up based on the size entry in
// the NT registry. This is the memory in Windows that is used by both the host application 
// such as LAPXM and the PLX PCI chip to pass data back and forth to the DSP. 
//
// NT Registry Entry:
// HKEY_Local_Machine\System\CurrentControlSet\Services\PCI9054\CommonBufferSize=0x8000000
//
// INPUT PARAMETER(s): None
//
// OUTPUT PARAMETER(s): None
//
// RETURN: Address         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////

PCI_MEMORY* PLX::GetPCIMemoryBaseAddress()
{
	return(&m_pPCI_Memory);

}

