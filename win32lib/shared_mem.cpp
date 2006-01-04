/********************************************************/
//
/********************************************************/

#include <Afx.h>

/* for shmem calls */
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <windows.h>

/* Allocate memory for a new shared memory buffer with name. */
/* Map the memory to prevent a protection fault */
/* Return a pointer to the memory */
void *shared_mem_create(char *name, int size)
   {
		HANDLE map;
		void *memory;

		// Create the shared memory object
		map = CreateFileMapping(INVALID_HANDLE_VALUE ,    // Current file handle. 
	    NULL,                           // Default security. 
	    PAGE_READWRITE,                 // Read/write permission. 
	    0,                              // Max. object size. 
	    size,                           // Size of hFile. 
	    name);							// Name of mapping object. 
		
		// Utter failure
		if (map == NULL) 
		{ 
		    printf("Could not create Shared Memory Object!\n");
			return NULL;
		}
		
		// Get a pointer to the memory
		memory = MapViewOfFile(map, // Handle to mapping object. 
		FILE_MAP_ALL_ACCESS,               // Read/write permission. 
		0,                                 // Max. object size. 
		0,                                 // Size of hFile. 
		0);                                // Map entire file. 
		
		// Failed
		if (memory == NULL) 
		{ 
		    printf("Could not map view of file.\n"); 
			return(NULL);
		} 
		
		// Set all the memory to NULL
		memset(memory,0,size);
		return(memory);
   }

int	shared_mem_exhist(char *name)
{
	HANDLE map;
	map = OpenFileMapping(FILE_MAP_ALL_ACCESS, // Read/write permission. 
    FALSE,                             // Do not inherit the name
    name);            // of the mapping object. 

	if (map == NULL)
		return(0);
	else
	{
		CloseHandle(map);
		return(1);
    }
}

/* Get a pointer to some previously opened memory */
/* Returns NULL if none available. Returns number of bytes in size */
void *shared_mem_open(char *name)
{
	HANDLE map = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,name);

	if(!map)
		return(NULL);
	
	// Utter failure
	if (map == NULL) 
	{ 
	    printf("Could not create Shared Memory Object!\n");
		return NULL;
	}
		
	// Get a pointer to the memory
	void *memory = MapViewOfFile(map, // Handle to mapping object. 
	FILE_MAP_ALL_ACCESS,               // Read/write permission. 
	0,                                 // Max. object size. 
	0,                                 // Size of hFile. 
	0);                                // Map entire file. 
		
	// Failed
	if (memory == NULL) 
	{ 
	    printf("Could not map view of file.\n"); 
		return(NULL);
	} 
	
	CloseHandle(map);
	return(memory);

}


/* close the shared memory object */
void shared_mem_close(char *name, void *addr, int size)
{
	HANDLE map;
	
	// Unmap the memory space
	UnmapViewOfFile(addr);

	// Get a handle to the file so we can close it
	map = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,name);
	CloseHandle(map);

}
   
/* get the size of the shared memory object */
int  shared_mem_size(char *name)
{
   return 23;
}
   