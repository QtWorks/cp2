//////////////////////////////////////////////////
void initDsp(void)
{ 
unsigned int *gcr, *csr,*sdr_cr,*sdr_tr;
int i; 
void delay(void);

/* setup EMIF Global and Local Control Registers */
/* this should allow external memory to be used! */
	gcr    = (unsigned int *)0x1800000;
	csr    = (unsigned int *)0x1800008; /* CE0 */ 
	sdr_cr = (unsigned int *)0x1800018;
	sdr_tr = (unsigned int *)0x180001c;
	
	*gcr = 0x6c; /* Piraq 3 values */

/* Configure SBSRAM */
	*csr = 0x40;  /* SBSRAM mapped to CEO */
	
/* Configure SDRAM */	
	csr  = (unsigned int *)0x1800010; /* CE2 */
	*csr = 0x30;  /* SDRAM mapped to CE2 */ 
	*sdr_cr = 0x7115000; /* 2 16bit SDRAMs; INIT=1 */
	for (i=0; i<10; i++)  /* Wait until SDRAM initializes */
    	delay();
	*sdr_tr = 0x4E1; 
		
/* Configure Asynchronous interface for CE1 */
/* Note may need to change when we try to configure PCI chip */
	csr = (unsigned int *)0x1800004; /* CE1 */
	*csr = 0x10910221;  /* async IF mapped to CE1 */
	
/* Configure Asynchronous interface for CE3 */
/* Note may need to change when we try to configure PCI chip */
	csr = (unsigned int *)0x1800014; /* CE3 */
	*csr = 0x14511121;  /* async IF mapped to CE3 - works for single + DMA */ 	 
}	          

//////////////////////////////////////////////////
/* generates a delay for led blinking */
void delay(void)
{
  volatile int i,j;
    
  j = 1;
  for (i=1; i<500000; i++)
    j += i;
}
    
