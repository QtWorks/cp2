/* PCI DMA Interrupt Service Routine */

#include <std.h>
#include <sem.h>
#include <sys.h>
#include <csl_irq.h>

#include "CP2Piraqcfg.h"
#include "local_defs.h"


extern volatile int FIFO_Available;
                                                 
void PciDmaISR(void)
   {
	volatile unsigned int *pci_cfg_ptr;
	unsigned int previousCE1;
	void med_dly(void);
   /* Re-configure Asynchronous interface for CE1 */
/* Increase cycle length to talk to PLX chip */

	previousCE1 = WriteCE1(PLX_CFG_MODE);	   

/* Clear DMA interrupt */	
	pci_cfg_ptr = (volatile unsigned int *)0x1400128;

	*pci_cfg_ptr = 0x8;           /* DMACSR0 */

/* Return Asynchronous interface for CE1 to initial settings */

	WriteCE1(previousCE1);
	SEM_post(&PciTransferFinished);
   }
