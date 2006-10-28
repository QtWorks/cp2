#include <stdafx.h>
#include <winsock2.h>
#include "CP2PIRAQ.h"
#include "get_julian_day.h"
#include "proto.h"

#define CYCLE_HITS 20

///////////////////////////////////////////////////////////////////////////
CP2PIRAQ::CP2PIRAQ(
				   CONFIG* pConfig_,
				   unsigned int Nhits_, 
				   int outputPort_, 
				   char* configFname_, 
				   char* dspObjFname,
				   int bytespergate_,
				   char* cmdFifoName_):
PIRAQ(),
pConfig(pConfig_), 
Nhits(Nhits_), 
outport(outputPort_), 
configFname(configFname_), 
dspObjFname(dspObjFname),
bytespergate(bytespergate_),
cmdFifoName(cmdFifoName_),
lastpulsenumber(0)
{
	if((outsock = open_udp_out("192.168.3.255")) ==  ERROR)			/* open one socket */
	{
		printf("Could not open output socket\n"); 
		exit(0);
	}
	printf("udp socket opens; outsock1 = %d\n", outsock); 

	init();
}

///////////////////////////////////////////////////////////////////////////
CP2PIRAQ::~CP2PIRAQ()
{
}

/////////////////////////////////////////////////////////////////////////////
int
CP2PIRAQ::init()	 
{
	int r_c;   // generic return code
	int y;

	r_c = this->Init(PIRAQ_VENDOR_ID,PIRAQ_DEVICE_ID); 
	printf("this->Init() r_c = %d\n", r_c); 

	if (r_c == -1) { // how to use GetErrorString() 
		char errmsg[256]; 
		this->GetErrorString(errmsg); printf("error: %s\n", errmsg); 
		return -1; 
	}

	/* put the DSP into a known state where HPI reads/writes will work */
	this->ResetPiraq(); // !!!redundant? 
	this->GetControl()->UnSetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);
	this->GetControl()->SetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);
	printf("this reset\n"); 
	unsigned int EPROM1[128]; 
	this->ReadEPROM(EPROM1);
	for(y = 0; y < 16; y++) {
		printf("%08lX %08lX %08lX %08lX\n",EPROM1[y*4],EPROM1[y*4+1],EPROM1[y*4+2],EPROM1[y*4+3]); 
	}

	this->SetCP2PIRAQTestAction(SEND_CHA);	//	send CHA by default; SEND_COMBINED after dynamic-range extension implemented 
	stop_piraq(pConfig, this);

	// create the data fifo.
	pFifo = (FIFO *)this->GetBuffer(); 

	// CP2: data packets sized at runtime.  + BUFFER_EPSILON
	piraq_fifo_init(
		pFifo,"/PRQDATA", 
		HEADERSIZE, 
		Nhits * (HEADERSIZE + (pConfig->gatesa * bytespergate) + BUFFER_EPSILON), 
		PIRAQ_FIFO_NUM); 
	printf("hit size = %d computed Nhits = %d\n", (HEADERSIZE + (pConfig->gatesa * bytespergate) + BUFFER_EPSILON), Nhits); 

	if (!pFifo) { 
		printf("this fifo_create failed\n"); exit(0);
	}
	printf("pFifo = %p, recordsize = %d\n", pFifo, pFifo->record_size); 

	pPkt = (PACKET *)fifo_get_header_address(pFifo); 

	pPkt->cmd.flag = 0; // Preset the flags just in case
	pPkt->data.info.channel = 0;			// set BOARD number

	struct_init(&pPkt->data.info, configFname);   /* initialize the info structure */
	_info = pPkt->data.info;
    _prt = pPkt->data.info.prt[0];   // get PRTs from valid packet as determined above. 

	r_c = this->LoadDspCode(dspObjFname); // load entered DSP executable filename
	printf("loading %s: this->LoadDspCode returns %d\n", dspObjFname, r_c);  
	timerset(pConfig, this); // !note: also programs pll and FIR filter. 
	printf("Opening FIFO %s", cmdFifoName); 
	pCmd = fifo_create(cmdFifoName,0,HEADERSIZE,CMD_FIFO_NUM);
	if(!pCmd)	{
		printf("\nCannot open %s FIFO buffer\n", cmdFifoName); 
		exit(-1);
	}
	pCmd->port = CMD_RING_PORT+1;
	/* make sure command socket is last file descriptor opened */
	if((cmd_notifysock = open_udp_in(pCmd->port)) == ERROR) { /* open the input socket on port where data will come from */
		printf("Could not open piraq notification socket\n"); 
		exit(-1);
	}
	printf("cmd_notifysock = %d \n", cmd_notifysock); 
	this->SetCP2PIRAQTestAction(SEND_CHA);	//	send CHA by default; SEND_COMBINED after dynamic-range extension implemented 
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
int 
CP2PIRAQ::start() 
{
	pPkt->data.info.pulse_num = pulsenum;	// set UNIX epoch pulsenum just before starting
	pPkt->data.info.beam_num = beamnum; 
	pPkt->data.info.packetflag = 1;			// set to piraq: get header! 
	printf("board%d: receiver_gain = %4.2f vreceiver_gain = %4.2f \n", 
		pPkt->data.info.channel, 
		pPkt->data.info.receiver_gain, 
		pPkt->data.info.vreceiver_gain); 
	printf("board%d: noise_power = %4.2f vnoise_power = %4.2f \n", 
		pPkt->data.info.channel, 
		pPkt->data.info.noise_power, 
		pPkt->data.info.vnoise_power); 
	// start the PIRAQ: also points the piraq to the fifo structure 
	if (!::start(pConfig, this, pPkt))
	{
		printf("Piraq DSP program not ready: pkt->cmd.flag != TRUE (1)\n");
		return -1;
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////////////
int
CP2PIRAQ::poll(int julian_day) 
{
	int i;
	int j;
	int k;
	struct timeval tv;
	int cur_fifo_hits;

	tv.tv_sec = 0;
	tv.tv_usec = 1000; 

	/* do polling */
	fd_set rfd;
	FD_ZERO(&rfd);
	FD_SET(cmd_notifysock, &rfd); //!!!Mitch eliminated but while(1) below depends on it. 

	int val = select(cmd_notifysock+1, &rfd, 0, 0, &tv);

	if (val < 0) 
	{
		printf( "select1 val = 0x%x\n", val ); 
		return 1;
	}

	else if(val == 0) { /* use time out for polling */
		// take CYCLE_HITS beams from piraq:
		while((
			(cur_fifo_hits = fifo_hit(pFifo)) > 0) && 
			(cycle_fifo_hits < CYCLE_HITS)) 
		{ // fifo hits ready: save #hits pending 
			cycle_fifo_hits++; 
			PACKET* pFifoPiraq = (PACKET *)fifo_get_read_address(pFifo, 0); 
			unsigned int hits = pFifoPiraq->data.info.hits; 
			int gates = pFifoPiraq->data.info.gates; 
			float scale = (float)(PIRAQ3D_SCALE*PIRAQ3D_SCALE*hits); // scale fifo data 
			udp = &pFifoPiraq->udp;
			udp->magic = MAGIC;
			udp->type = UDPTYPE_PIRAQ_CP2_TIMESERIES; 
			pFifoPiraq->data.info.bytespergate = 2 * (sizeof(float)); // Staggered PRT ABPDATA

			unsigned __int64 temp = 
				pFifoPiraq->data.info.pulse_num * (unsigned __int64)(_prt * (float)COUNTFREQ + 0.5);
			pFifoPiraq->data.info.secs = 
				temp / COUNTFREQ;
			pFifoPiraq->data.info.nanosecs = 
				((unsigned __int64)10000 * (temp % ((unsigned __int64)COUNTFREQ))) 
				/ (unsigned __int64)COUNTFREQ;
			pFifoPiraq->data.info.nanosecs *= 
				(unsigned __int64)100000; // multiply by 10**5 to get nanoseconds

			az += 0.5;  
			if(az > 360.0) { // full scan 
				az -= 360.0; // restart azimuth angle 
				el += 7.0;		// step antenna up 
				scan++; // increment scan count
				if (el >= 21.0) {	// beyond allowed step
					el = 0.0; // start over at horizon
					volume++; // finish volume
				}
			} 
			pFifoPiraq->data.info.az = az;  
			pFifoPiraq->data.info.el = el; // set in packet 
			pFifoPiraq->data.info.scan_num = scan;
			pFifoPiraq->data.info.vol_num = volume;  
			//					printf("fake: az = %4.3f el = %4.3f\n",fifopiraq1->data.info.az,fifopiraq1->data.info.el); 
			// ... to here 
#ifdef TIME_TESTING	
			// for mSec-resolution time tests: 
			_ftime( &timebuffer );
			timeline = ctime( & ( timebuffer.time ) );
			printf( "1: %hu ... fakin it.\n", timebuffer.millitm );
#endif
			pFifoPiraq->data.info.julian_day = julian_day; 

#ifdef	DRX_PACKET_TESTING	// define to activate data packet resizing for throughput testing. 
			//					fifopiraq1->data.info.recordlen = (fifopiraq1->data.info.clutter_end[0])*fifopiraq1->data.info.gates*24 + (RECORDLEN(fifopiraq1)); // vary multiplier; note CPU usage in Task Manager. 24 = 2*bytespergate. 
			pFifoPiraq->data.info.recordlen = Nhits*(RECORDLEN(pFifoPiraq)+BUFFER_EPSILON); /* this after numgates corrected */
#else
			pFifoPiraq->data.info.recordlen = RECORDLEN(pFifoPiraq); /* this after numgates corrected */
#endif
			// scale data -- offload from piraq: 
			//					fsrc = (float *)pFifoPiraq->data.data; 

			__int64 * __int64_ptr;
			__int64 * __int64_ptr2; 
			unsigned int * uint_ptr; 
			float * fsrc2; 
			for (i = 0; i < Nhits; i++) { // all hits in the packet 
				// compute pointer to datum in an individual hit, dereference and print. 
				// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config1->gatesa * bytespergate) + BUFFER_EPSILON)
				__int64_ptr = (__int64 *)((char *)&pFifoPiraq->data.info.beam_num + i*((HEADERSIZE + (pConfig->gatesa * bytespergate) + BUFFER_EPSILON))); 
				beamnum = *__int64_ptr; 
				__int64_ptr2 = (__int64 *)((char *)&pFifoPiraq->data.info.pulse_num + i*((HEADERSIZE + (pConfig->gatesa * bytespergate) + BUFFER_EPSILON))); 
				pulsenum = *__int64_ptr2; 
				uint_ptr = (unsigned int *)((char *)&pFifoPiraq->data.info.hits + i*((HEADERSIZE + (pConfig->gatesa * bytespergate) + BUFFER_EPSILON))); 
				j = *uint_ptr; 
				uint_ptr = (unsigned int *)((char *)&pFifoPiraq->data.info.channel + i*((HEADERSIZE + (pConfig->gatesa * bytespergate) + BUFFER_EPSILON))); 
				k = *uint_ptr; 
				fsrc2 = (float *)((char *)pFifoPiraq->data.data + i*((HEADERSIZE + (pConfig->gatesa * bytespergate) + BUFFER_EPSILON)));
				if (lastpulsenumber != pulsenum - 1) { // PNs not sequential
					printf("hit%d: lastPN = %I64d PN = %I64d\n", i+1, lastpulsenumber, pulsenum);  
					PNerrors++; 
				}
				lastpulsenumber = pulsenum; // previous hit PN
			}
			fifo_hits++;   
			// increase udp-send totalsize to Nhits
			int test_totalsize1 = Nhits*(TOTALSIZE(pFifoPiraq) + BUFFER_EPSILON); 
			pFifoPiraq->udp.totalsize = test_totalsize1; // CP2 throughput testing

			seq = send_udp_packet(outsock, outport, seq, udp); 
			fifo_increment_tail(pFifo);
		} // end	while(fifo_hit()
		cycle_fifo_hits = 0; // clear cycle counter 
	} // end	else if(val == 0) 

	return 0;
}
///////////////////////////////////////////////////////////////////////////
float
CP2PIRAQ::prt()
{
	return _prt;
}

INFOHEADER
CP2PIRAQ::info()
{
	return _info;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
