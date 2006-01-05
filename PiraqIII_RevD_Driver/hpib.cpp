/////////////////////////////////////////////////////////////////////////////////////////
// FILE NAME: HPIB.cpp
//
// DESCRIPTION: HPIB Class implementation. This class handles the HPIB interface
// to the DSP.
//
/////////////////////////////////////////////////////////////////////////////////////////
#include "HPIB.h"

/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: HPIB::HPIB(unsigned int *pBaseAddress)
//
// DESCRIPTION: Constructor - Sets up the addresses for the HPIB interface 
//
// INPUT PARAMETER(s): 
// unsigned int *pBaseAddress - Base address of the Piraq board
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
HPIB::HPIB(unsigned int *pBaseAddress)
{
	// Set Addresses
	m_pBaseAddress = (unsigned int*) pBaseAddress;

	m_pHPICLO           = ((unsigned short *)((unsigned char *)m_pBaseAddress + 0x1800));
	m_pHPICHI           = ((unsigned short *)((unsigned char *)m_pBaseAddress + 0x1802));
	m_pHPIALO           = ((unsigned short *)((unsigned char *)m_pBaseAddress + 0x1804));
	m_pHPIAHI           = ((unsigned short *)((unsigned char *)m_pBaseAddress + 0x1806));
	//set up non-autoincrement
	m_pHPIDLO           = ((unsigned short *)((unsigned char *)m_pBaseAddress + 0x180C));
	m_pHPIDHI           = ((unsigned short *)((unsigned char *)m_pBaseAddress + 0x180E));
	//set up autoincrement
	m_pHPIDLA           = ((unsigned short *)((unsigned char *)m_pBaseAddress + 0x1808));
	m_pHPIDHA           = ((unsigned short *)((unsigned char *)m_pBaseAddress + 0x180A));

}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: HPIB::~HPIB()
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
HPIB::~HPIB()
{
  
}






/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: HPIB::StartDsp()
//
// DESCRIPTION: Starts the DSP running 
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
void HPIB::StartDsp()
{		
	// Start the DSP by setting the bit in the hpib buss control word
	*m_pHPICLO = *m_pHPICLO | HINT;
	*m_pHPICHI = *m_pHPICLO | HINT;
}






/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: HPIB::coffload( char *pFilePathAndName, char* cErrorString)
//
// DESCRIPTION: Loads COFF Type 2 DSP code.
//
// INPUT PARAMETER(s): 
// char *pFilePathAndName - Full path and file name for the DSP code to load
//
// OUTPUT PARAMETER(s): 
// char* cErrorString - Error String describing failure
//
// RETURN: none         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
long  HPIB::coffload( char *pFilePathAndName, char* cErrorString)
{
	char cSectionName[9] = "";
	long lEntryAddress = 0;
	FILE* pInFile;
	long lSectionlength = 0;
	long lRemaining = 0;
	struct FileHeaderStructure FileHeader;
	struct SectionHeaderStructure SectionHeader;
	struct InputBufferStructure InBuffer;
	long lCurrentLocation = 0;
	m_FirstHpi = 1;


	//**************** Clear Memory **************************
	for (int i=0; i<STRMAX; i++)
	{
		InBuffer.data[i] = 0x0000;
	}
	for(lSectionlength=0; lSectionlength<0x7fff; lSectionlength += lRemaining)
	{
		// compute how much data to read 
		lRemaining = 0x7fff - lSectionlength;

		if(lRemaining >= 2000)
		{
		 lRemaining = 2000;
		}

		lReturnValue = Download(&InBuffer, (unsigned long)(lSectionlength), (unsigned long)lRemaining);
		if(lReturnValue != 0)
		{
			strcpy(cErrorString,"Could not clear DSP code space.");
			return (-1);
		}
	}


	//**************** Read the file header **************************

	// If 'pFilePathAndName' does not exist then return error 
	if((pInFile = fopen(pFilePathAndName, "rb")) == NULL)
	{
		strcpy(cErrorString,"Could not find:");
		strcat(cErrorString, pFilePathAndName); 
		return(-1);
	}

	fread(&FileHeader, sizeof(FileHeader),1,pInFile);

	// Skip optional header, if any 

	//**************** Read optional file header is one exhists **************************
	if(FileHeader.optbytes)
	{
		 // Set the file position for "pInFile" to optbytes-2 characters from 
		 // where it was left off by the previous fread statement 
		 fseek(pInFile,FileHeader.optbytes-2, SEEK_CUR);
	}
	
	// Get section header 

	//**************** Loop on the number of sections **************************
	for(i=1; i<=FileHeader.sections; i++)
	{
		// For the total number of sections in the input file do the following 

		//**************** Read in the section header **************************
		fread(&SectionHeader, sizeof(SectionHeader),1, pInFile);

		//**************** Get the section name **************************

		// Copy 8 characters from the 'name' field of struct 'SectionHeader' into file 'cSectionName' 
		strncpy(cSectionName,SectionHeader.name,8);

		// Terminate the string with a null char 
		strncat(cSectionName,"\0",1);

		//**************** Get the section size **************************

		if(SectionHeader.rawdataptr == 0x0)
		{
			// Determining section size 
			SectionHeader.sectsize =0;
		}

		// Enter loop if it is the first iteration and if the name of the section is ".text" 
		if ((!strcmp(SectionHeader.name,".text")) && i==1)
		{
			lEntryAddress = SectionHeader.phyaddr;
		}


		// This covers:
		// - the FileHeader length,
		// - the optional FileHeader length
		// - the length of the SectionHeader for the number of
		//    sections read (reflected by 'i' value of loop)- (-2)
		//    to give the current section ptr location 
		lCurrentLocation = sizeof(FileHeader) + FileHeader.optbytes + (i * sizeof(SectionHeader)) - 2;

		fseek(pInFile,SectionHeader.rawdataptr,SEEK_SET);

		if(SectionHeader.sectsize > 0 && strcmp(SectionHeader.name,".xref"))
		{
			// If the sectionsize is non-zero and 
			// if the sectionname is not ".xref", then enter the loop 

			//**************** Read in the raw section data **************************

			for(lSectionlength=0; lSectionlength<SectionHeader.sectsize; lSectionlength += lRemaining)
			{
				// Compute how much data to read 
				lRemaining = SectionHeader.sectsize - lSectionlength;

				if(lRemaining >= STRMAX)
				{
				 lRemaining = STRMAX;
				}

				// Read it 
				fread(&InBuffer, lRemaining*2, 1, pInFile);

				// Load it into memory 
				lReturnValue = Download(&InBuffer, (unsigned long)(SectionHeader.phyaddr+lSectionlength), (unsigned long)lRemaining);
				if(lReturnValue != 0)
				{
					strcpy(cErrorString,"CRC did not match.");
					return (-1);
				}
			}
       
		}

		fseek(pInFile,lCurrentLocation, SEEK_SET);	
	}

	fclose(pInFile);

	return (0);
}








/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: HPIB::Download (struct buffer *aData, unsigned long lDestinationAddress, unsigned long lLength)
//
// DESCRIPTION: Writes data into DSP 
//
// INPUT PARAMETER(s): 
// struct buffer *aData - Array of data to load
// unsigned long lDestinationAddress - Destination address to load into
// unsigned long lLength - Length of data to load
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
long HPIB::Download (struct InputBufferStructure *aData, unsigned long lDestinationAddress, unsigned long lLength)
{
	unsigned long lDataWord;
	unsigned long lCRC_In=0;
	unsigned long lCRC_Out=0;

	// Write data to the DSP 
	unsigned long lAddress = lDestinationAddress;
	for (unsigned int i=0; i<lLength/2; i+=2,lAddress+=4)
	{
		lDataWord  = (int)aData->data[i] + (((int)aData->data[1+i]) << 16);
		lCRC_In+=lDataWord;
		WriteLocation( lAddress, lDataWord );
	}

	// Read the data back from the DSP
	lAddress = lDestinationAddress;
	for (i=0; i<lLength/2; i+=2,lAddress+=4)
	{
		lCRC_Out+= ReadLocation(lAddress);
	}

	// Check to make sure that it was written correct
	if(lCRC_In != lCRC_Out)
	{
		return (-1);
	}

	return (0);
}







/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: HPIB::ReadLocation( unsigned int iAddress)
//
// DESCRIPTION: Constructor 
//
// INPUT PARAMETER(s): 
// unsigned int address - Location to read from
//
// OUTPUT PARAMETER(s): None
//
// RETURN: Data from location         
//
// WRITTEN BY: Coy Chanders 02-01-02
//
// LAST MODIFIED BY:
//
/////////////////////////////////////////////////////////////////////////////////////////
unsigned long HPIB::ReadLocation( unsigned int iAddress)
{
	SHORT2LONG   data;

	// Read 32 bit address
	data.lohi = (unsigned int)iAddress;

	if(m_FirstHpi)
	{
		m_FirstHpi = 0;
		
		// Write HPIC 1st halfword (with HHWIL low)
		*m_pHPICLO = HWOB;        
		
		// Write HPIC 2nd halfword (with HHWIL high)
		*m_pHPICHI = HWOB;        
	}

	// Write HPIC 2nd halfword (with HHWIL low)
	*m_pHPIALO = data.LO;
	
	// Write HPIC 2nd halfword (with HHWIL high) 
	*m_pHPIAHI = data.HI;    

	// Read Data
	data.LO = *m_pHPIDLO;
	data.HI = *m_pHPIDHI;

	return(data.lohi);

}








/////////////////////////////////////////////////////////////////////////////////////////
// METHOD NAME: HPIB::WriteLocation(unsigned int iAddress, unsigned int iData)
//
// DESCRIPTION: Writes data into address location in the DSP memory space via the HPIB 
// This routine does not do autoincrement 
//
// INPUT PARAMETER(s): 
// unsigned int iAddress - 32 bit DSP relative address 
// unsigned int iData - Data to write
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
void HPIB::WriteLocation(unsigned int iAddress, unsigned int iData)
{
	SHORT2LONG   data;

	// Write 32 bit address
	data.lohi = iAddress;

	 // Init HWOB = 1 and HPIA 
	if(m_FirstHpi)
	{
		m_FirstHpi = 0;

		// Write HPIC 1st halfword (with HHWIL low)
		*m_pHPICLO = HWOB;        

		// Write HPIC 2nd halfword (with HHWIL high)
		*m_pHPICHI = HWOB;       
	}

	// write HPIC 2nd halfword (with HHWIL low) 
	*m_pHPIALO = data.LO;    

	// write HPIC 2nd halfword (with HHWIL high) 
	*m_pHPIAHI = data.HI;    

	// Write Data
	data.lohi = iData;

	*m_pHPIDLO = data.LO;
	*m_pHPIDHI = data.HI;
};





