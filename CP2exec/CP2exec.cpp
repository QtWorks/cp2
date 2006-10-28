#include <stdafx.h> //  this gets getch() and kbhit() included ... 
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include <windows.h>
#include <winsock2.h>
#include <mmsystem.h>
#include <sys/timeb.h>
#include <time.h>

#include "CP2exec.h" 
#include "CP2PIRAQ.h"
#include "get_julian_day.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define			TIME_TESTING		// define to activate millisecond printout for time of events. 
#ifdef CP2_TESTING		// switch ON test code for CP2 
// test drx data throughput limits by varying data packet size w/o changing DSP computational load:  
#define			DRX_PACKET_TESTING	// define to activate data packet resizing for CP2 throughput testing. 
#endif
#define			CYCLE_HITS	20	// #fifo hits to take from piraq-host shared memory before rotating to next board: CP2

//#define TESTING_TIMESERIES // compute test diagnostic timeseries data in one of two piraq channels: 
//#define TESTING_TIMESERIES_RANGE	// test dynamic reassignment of timeseries start, end gate using 'U','D'

#define PIRAQ3D_SCALE	1.0/pow(2,31)	// normalize 2^31 data to 1.0 full scale using multiplication

FILE * db_fp; // debug data file 

int keyvalid() 
{
	while(!_kbhit()) { } // no keystroke 
	getch();    // get it
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
int
init_piraq(PIRAQ* &pPiraq, FIFO* &pCmd, char* cmdFifoName, FIFO* &pFifo, CONFIG* pConfig, 
		   PACKET* &pPkt, int &cmd_notifysock, unsigned int Nhits, 
		   int bytespergate, char* configFname, char* dspObjFname) 
{

	int r_c;   // generic return code
	int y;

	// create a new PIRAQ
	pPiraq = new PIRAQ;

	r_c = pPiraq->Init(PIRAQ_VENDOR_ID,PIRAQ_DEVICE_ID); 
	printf("pPiraq->Init() r_c = %d\n", r_c); 

	if (r_c == -1) { // how to use GetErrorString() 
		char errmsg[256]; 
		pPiraq->GetErrorString(errmsg); printf("error: %s\n", errmsg); 
		return -1; 
	}

	/* put the DSP into a known state where HPI reads/writes will work */
	pPiraq->ResetPiraq(); // !!!redundant? 
	pPiraq->GetControl()->UnSetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);
	pPiraq->GetControl()->SetBit_StatusRegister0(STAT0_SW_RESET);
	Sleep(1);
	printf("pPiraq reset\n"); 
	unsigned int EPROM1[128]; 
	pPiraq->ReadEPROM(EPROM1);
	for(y = 0; y < 16; y++) {
		printf("%08lX %08lX %08lX %08lX\n",EPROM1[y*4],EPROM1[y*4+1],EPROM1[y*4+2],EPROM1[y*4+3]); 
	}

	pPiraq->SetCP2PIRAQTestAction(SEND_CHA);	//	send CHA by default; SEND_COMBINED after dynamic-range extension implemented 
	stop_piraq(pConfig, pPiraq);

	// create the data fifo.
	pFifo = (FIFO *)pPiraq->GetBuffer(); 

	// CP2: data packets sized at runtime.  + BUFFER_EPSILON
	piraq_fifo_init(
		pFifo,"/PRQDATA", 
		HEADERSIZE, 
		Nhits * (HEADERSIZE + (pConfig->gatesa * bytespergate) + BUFFER_EPSILON), 
		PIRAQ_FIFO_NUM); 
	printf("hit size = %d computed Nhits = %d\n", (HEADERSIZE + (pConfig->gatesa * bytespergate) + BUFFER_EPSILON), Nhits); 

	if (!pFifo) { 
		printf("pPiraq fifo_create failed\n"); exit(0);
	}
	printf("pFifo = %p, recordsize = %d\n", pFifo, pFifo->record_size); 

	pPkt = (PACKET *)fifo_get_header_address(pFifo); 

	pPkt->cmd.flag = 0; // Preset the flags just in case
	pPkt->data.info.channel = 0;			// set BOARD number
	struct_init(&pPkt->data.info, configFname);   /* initialize the info structure */
	r_c = pPiraq->LoadDspCode(dspObjFname); // load entered DSP executable filename
	printf("loading %s: pPiraq->LoadDspCode returns %d\n", dspObjFname, r_c);  
	timerset(pConfig, pPiraq); // !note: also programs pll and FIR filter. 
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
	pPiraq->SetCP2PIRAQTestAction(SEND_CHA);	//	send CHA by default; SEND_COMBINED after dynamic-range extension implemented 
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
int 
start_piraq(PIRAQ* pPiraq, CONFIG* pConfig, PACKET* pPkt, __int64 pulsenum, __int64 beamnum) 
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
	if (!start(pConfig,pPiraq,pPkt))
	{
		printf("Piraq DSP program not ready: pkt->cmd.flag != TRUE (1)\n");
		return -1;
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////////////
int
poll_piraq(
		   CONFIG* pConfig,
		   FIFO* &pFifo,  
		   UDPHEADER* &udp, 
		   int &cmd_notifysock, 
		   int beamnum, 
		   int pulsenum,
		   int outport, 
		   int outsock,
		   int &cycle_fifo_hits, 
		   int &fifo_hits, 
		   __int64 &lastpulsenumber, 
		   int &PNerrors, 
		   float &az, 
		   float &el, 
		   unsigned int &scan, 
		   unsigned int &volume, 
		   unsigned int &seq, 
		   float prt,
		   unsigned int julian_day,
		   int bytespergate,
		   unsigned int Nhits
		   ) 
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
				pFifoPiraq->data.info.pulse_num * (unsigned __int64)(prt * (float)COUNTFREQ + 0.5);
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
					fprintf(db_fp, "%d:hit%d: lastPN = %I64d PN = %I64d\n", 
						pFifoPiraq->data.info.channel, i+1, lastpulsenumber, pulsenum); 
				}
				lastpulsenumber = pulsenum; // previous hit PN
			}
			fifo_hits++;   
#ifndef DRX_PACKET_TESTING	// define activates data packet resizing for CP2 throughput testing. 
			pFifoPiraq->udp.totalsize = TOTALSIZE(fifopiraq1); // ordinary operation
#else	// increase udp-send totalsize to Nhits
			int test_totalsize1 = Nhits*(TOTALSIZE(pFifoPiraq) + BUFFER_EPSILON); 
			//works ... int test_totalsize1 = Nhits*(TOTALSIZE(fifopiraq1) + BUFFER_EPSILON) + 6*sizeof(UDPHEADER); // add Nhits-1 udpsend adds 1 more ... fuckinay, man
			pFifoPiraq->udp.totalsize = test_totalsize1; // CP2 throughput testing
#endif
			seq = send_udp_packet(outsock, outport, seq, udp); 
			fifo_increment_tail(pFifo);
		} // end	while(fifo_hit()
		cycle_fifo_hits = 0; // clear cycle counter 
	} // end	else if(val == 0) 

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
int 
_tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	CP2PIRAQ* piraq1;
	CP2PIRAQ* piraq2;
	CP2PIRAQ* piraq3;

	CONFIG *config1, *config2, *config3;
//	FIFO *fifo1, *fifo2, *fifo3; 
//	FIFO *cmd1, *cmd2, *cmd3; 
//	PACKET *fifopiraq2, *fifopiraq3;
//	PACKET *pkt1, *pkt2, *pkt3, *pn_pkt; 
//	UDPHEADER *udp1, *udp2, *udp3;

//	int cmd1_notifysock, cmd2_notifysock, cmd3_notifysock; 
//	int outsock1, outsock2, outsock3; 
//	unsigned __int64 temp2, temp3; 

//	float az1 = 0, az2 = 0, az3 = 0;
//	float el1 = 0, el2 = 0, el3 = 0;
//	unsigned int scan1 = 0, scan2 = 0, scan3 = 0;
//	unsigned int volume1 = 0, volume2 = 0, volume3 = 0; 
//	int fifo1_hits, fifo2_hits, fifo3_hits; // cumulative hits per board 
//	int cycle_fifo1_hits, cycle_fifo2_hits, cycle_fifo3_hits; // current hits per cycle 
	char fname1[10]; char fname2[10]; char fname3[10]; // configuration filenames
//	__int64 lastpulsenumber1, lastpulsenumber2, lastpulsenumber3;
//	__int64 lastbeamnumber1, lastbeamnumber2, lastbeamnumber3;
//	int  PNerrors1, PNerrors2, PNerrors3; 
//	unsigned int seq1, seq2, seq3; 


	//for mSec-resolution time tests: 
	struct _timeb timebuffer;
	char *timeline;

	// set/compute #hits combined by piraq: equal in both piraq executable (CP2_DCCS3_1.out) and CP2exec.exe 
	unsigned int Nhits; 
	//unsigned int packets = 0; 


	int i,j,k,y,outport;
	char c,name[80];
	unsigned int errors = 0; 
	int r_c; // return code 
	int piraqs = 0;   // board count -- default to single board operation 
//	cycle_fifo1_hits = cycle_fifo2_hits = cycle_fifo3_hits = 0; // clear hits per cycle     
	FILE * dspEXEC; 
	//	int dspl_hits = 100; // fifo hits modulus for updating display 
	unsigned int bytespergate; 
	__int64 pulsenum, beamnum; 
	time_t now, now_was; 
	int	PIRAQadjustAmplitude = 0; 
	int	PIRAQadjustFrequency = 1; 
	unsigned int julian_day; 
	TIMER ext_timer; // structure defining external timer parameters 
	int iError;


//	lastpulsenumber1 = lastpulsenumber2 = lastpulsenumber3 = 0;
//	lastbeamnumber1 = lastbeamnumber2 = lastbeamnumber3 = 0;
//	PNerrors1 = PNerrors2 = PNerrors3 = 0; 

	config1	= new CONFIG; 
	config2	= new CONFIG; 
	config3	= new CONFIG; 

	// open debug data file: 
	db_fp = fopen("debug.dat","w");
	fprintf(db_fp,"CP2exec.exe results:\n");  

	if (argc < 4) {
		printf("CP2exec <DSP filename> <rcv data format: 'f' floats, 's' unsigned shorts> <piraq mask> [config file]\n"); 
		exit(1);
	}

	if ((dspEXEC = fopen(argv[1],"r")) == NULL) // DSP executable file not found 
	{ printf("Usage: %s <DSP filename> DSP executable file not found\n", argv[0]); exit(0); 
	} 
	fclose(dspEXEC); // existence test passed; use command-line filename

	printf("file %s will be loaded into piraq\n", argv[1]); 

	piraqs = atoi(argv[3]); 
	printf("piraq select mask = %d\n", piraqs); 

	if (argc > 4) { // entered a filename
		strcpy(fname1, argv[4]);
		strcpy(fname2, argv[4]);
		strcpy(fname3, argv[4]);
	}
	else {
		strcpy(fname1, "config1");	printf(" config1 filename %s will be used\n", fname1); 
		strcpy(fname2, "config2");	printf(" config2 filename %s will be used\n", fname2); 
		strcpy(fname3, "config3");	printf(" config3 filename %s will be used\n", fname3); 
	}

	printf("\n\nTURN TRANSMITTER OFF for piraq dc offset measurement.\nPress any key to continue.\n"); 
	while(!kbhit())	;
	c = toupper(getch()); // get the character

	outport = 3100; 

	// open sockets
	//	if((outsock1 = open_udp_out("192.168.3.255")) ==  ERROR)			/* open one socket */
	//	{
	//		printf("%s: Could not open output socket 1\n",name); 
	//		exit(0);
	//	}
	//	printf("udp socket opens; outsock1 = %d\n", outsock1); 

//	if((outsock2 = open_udp_out("192.168.3.255")) ==  ERROR)			/* open second socket */
//	{
//		printf("%s: Could not open output socket 2\n",name); 
//		exit(0);
//	}
//	printf("udp socket opens; outsock2 = %d\n", outsock2); 
//	if((outsock3 = open_udp_out("192.168.3.255")) ==  ERROR)			/* open second socket */
//	{
//		printf("%s: Could not open output socket 3\n",name); 
//		exit(0);
//	}
//	printf("udp socket opens; outsock3 = %d\n", outsock3); 

	timer_stop(&ext_timer); // stop timer card 

	// read in fname.dsp, or use configX.dsp if NULL passed. set up all parameters
	readconfig(fname1,config1);    
	readconfig(fname2,config2);    
	readconfig(fname3,config3);   

	if (config1->dataformat == 18) { // CP2 Timeseries 
		bytespergate = 2 * sizeof(float); 
		// CP2: compute #hits combined into one PCI Bus transfer
		Nhits = 65536 / (HEADERSIZE + (config1->gatesa * bytespergate) + BUFFER_EPSILON); 
		if	(Nhits % 2)	//	computed odd #hits
			Nhits--;	//	make it even
	}
	else	
	{	//	no other dataformats supported
		printf("dataformat != 18 not supported\n"); 
		exit(0); 
	}

	///////////////////////////////////////////////////////////////////////////
	//
	//    Create piraqs

	float prt;
	INFOHEADER info;
	if (piraqs & 1) {
		piraq1 = new CP2PIRAQ(config1, Nhits, outport, fname1, argv[1], bytespergate, "/CMD1");
		prt = piraq1->prt();
		info = piraq1->info();
	}

	if (piraqs & 2) {
		piraq2 = new CP2PIRAQ(config2, Nhits, outport+1, fname2, argv[1], bytespergate, "/CMD2");
		prt = piraq2->prt();
		info = piraq2->info();
	}

	if (piraqs & 4) {
		piraq3 = new CP2PIRAQ(config3, Nhits, outport+2, fname3, argv[1], bytespergate, "/CMD3");
		prt = piraq3->prt();
		info = piraq3->info();
	}


	///////////////////////////////////////////////////////////////////////////
	//
	//      Don't know what the following stuff is all about

	__int64 prf; 
	prf = (__int64)(ceil((double)(1/prt)));
	unsigned int pri; 
	// fp: 
	double fpprf, fpsuppm; 
	fpprf = ((double)1.0)/((double)prt); 
	//!board-specific for 2,3
	//	hits = config1->hits; 
	fpsuppm = fpprf/(double)config1->hits; 
	//		fpExactmSecperBeam = ((double)1000.0)/fpsuppm; 
	printf("prt = %+8.3e fpprf = %+8.3e fpsuppm = fpprf/hits = %+8.3e\n", prt, fpprf, fpsuppm); 
	//		printf("fpExactmSecperBeam = %8.3e\n", fpExactmSecperBeam); 

	pri = (unsigned int)(((float)COUNTFREQ)/(float)(1/prt)) + 0.5; 
	printf("pri = %d\n", pri); 
	printf("prf = %I64d\n", prf); 
	//!board-specific for 2,3
	//	hits = config1->hits; 
	printf("hits = %d\n", config1->hits); 
	float suppm; suppm = prf/(float)config1->hits; 
	printf("ceil(prf/hits) = %4.5f, prf/hits = %4.5f\n", ceil(prf/(float)config1->hits), suppm); 
	//		if (ceil(prf/hits) != suppm) {
	//			printf("integral beams/sec required\n"); exit(0); 
	//		} 

	// get current second and wait for it to pass; 
	now = time(&now); now_was = now;
	while(now == now_was) // current second persists 
		now = time(&now);	//  
	now = time(&now); now_was = now;
	pulsenum = ((((__int64)(now+2)) * (__int64)COUNTFREQ) / pri) + 1; 
	beamnum = pulsenum / config1->hits;
	printf("pulsenum=%I64d\n", pulsenum); 
	printf("beamnum=%I64d\n", beamnum); 

	///////////////////////////////////////////////////////////////////////////
	//
	//      start the piraqs, waiting for each to indicate functionality

	if (piraqs & 0x01) { // turn on slot 1
		if (piraq1->start()) 
			exit(-1);
	} 
	if (piraqs & 0x02) { // turn on slot 2
		if (piraq2->start()) 
			exit(-1);
	} 
	if (piraqs & 0x04) { // turn on slot 3
		if (piraq3->start()) 
			exit(-1);
	} 

	// get current second and wait for it to pass; 
	now = time(&now); now_was = now;
	while(now == now_was) // current second persists 
		now = time(&now);	//  
	now = time(&now); now_was = now;
	// compute pulsenumber when data acquisition will begin: 
	// increment current second to account for wait following
	pulsenum = ((((__int64)(now+1)) * (__int64)COUNTFREQ) / pri) + 1; 
	beamnum = pulsenum / config1->hits;  
	printf("now+1=%d: data collection starts THEN.\n... waiting for this second (now) to expire ... \npulsenumber computed using NEXT second, when timer board is triggered \n", now+1);  
	printf("now=%d\n", now);  
	printf("pri = %d\n", pri+1); 
	printf("pulsenum=%I64d\n", pulsenum); 
	printf("beamnum=%I64d\n", beamnum); 

	// time-related parameters initialized -- wait for new second then start timer card and 
	// data acquisition. 
	printf("now=%d: ... still waiting for this second (now) to expire ... then timer will start\n\n", now);  
	while(now == now_was) // current second persists 
		now = time(&now); 
	printf("now=%d: ... now_was=%d\n", now, now_was);  

	// start timer board immediately:
	start_timer_card(&ext_timer, &info); // pn_pkt = PACKET pointer to a live piraq 

	///////////////////////////////////////////////////////////////////////////
	//
	//      poll the piraqs in succesion

	// all running -- get data!
//	fifo1_hits = 0; fifo2_hits = 0; fifo3_hits = 0; // 
//	seq1 = seq2 = seq3 = 0; // initialize sequence# for each channel 
	while(1) { // until 'q' 
		julian_day = get_julian_day(); 

		if (piraqs & 0x01) { // turn on slot 1
			if (piraq1->poll(julian_day))
				continue;
		}

		if (piraqs & 0x02) { // turn on slot 1
			if (piraq2->poll(julian_day))
				continue;
		}

		if (piraqs & 0x04) { // turn on slot 1
			if (piraq3->poll(julian_day))
				continue;
		}


		if (kbhit()) {
			c = toupper(getch());
#ifdef TESTING_TIMESERIES_RANGE
			if(c == 'U') {
				pn_pkt->data.info.ts_start_gate += 1;
				pn_pkt->data.info.ts_end_gate += 1;
			}
			if(c == 'D') {
				pn_pkt->data.info.ts_start_gate -= 1;
				pn_pkt->data.info.ts_end_gate -= 1;
			}
#endif
			// !!!DO LATER: leave these here but do not use for now. 
			//				if(c == '+')		TimerStartCorrection += (__int64)ExactmSecperBeam; 
			//				if(c == '-')		TimerStartCorrection -= (__int64)ExactmSecperBeam; 
#ifndef TESTING_TIMESERIES
			//				if(c == '+')		TimerStartCorrection += (__int64)mSecperBeam; 
			//				if(c == '-')		TimerStartCorrection -= (__int64)mSecperBeam;
			if(c == 'A')		{ PIRAQadjustAmplitude = 1; PIRAQadjustFrequency = 0; printf("'U','D','+','-' adjust test waveform amplitude\n"); }
			if(c == 'W')		{ PIRAQadjustAmplitude = 0; PIRAQadjustFrequency = 1; printf("'U','D','+','-' adjust test waveform frequency\n"); }
			//	adjust test data freq. up/down fine/coarse
			if(PIRAQadjustFrequency)	{	//	use these keys to adjust the test sine frequency
				if(c == '+')		piraq1->SetCP2PIRAQTestAction(INCREMENT_TEST_SINUSIOD_FINE);	
				if(c == '-')		piraq1->SetCP2PIRAQTestAction(DECREMENT_TEST_SINUSIOD_FINE);
				if(c == 'U')		piraq1->SetCP2PIRAQTestAction(INCREMENT_TEST_SINUSIOD_COARSE);	 
				if(c == 'D')		piraq1->SetCP2PIRAQTestAction(DECREMENT_TEST_SINUSIOD_COARSE);	
			}
			//	adjust test data amplitude: up/down fine/coarse
			if(PIRAQadjustAmplitude)	{	//	use these keys to adjust the test sine amplitude
				if(c == '+')		piraq1->SetCP2PIRAQTestAction(INCREMENT_TEST_AMPLITUDE_FINE);	
				if(c == '-')		piraq1->SetCP2PIRAQTestAction(DECREMENT_TEST_AMPLITUDE_FINE);
				if(c == 'U')		piraq1->SetCP2PIRAQTestAction(INCREMENT_TEST_AMPLITUDE_COARSE);	 
				if(c == 'D')		piraq1->SetCP2PIRAQTestAction(DECREMENT_TEST_AMPLITUDE_COARSE);	
			}
#else
			// adjust test timeseries power: !note requires NOT TESTING_TIMESERIES_RANGE. 
			if(c == 'U')		test_ts_power *= sqrt(10.0); 
			if(c == 'D')		test_ts_power /= sqrt(10.0); // factor of arg in power
			// adjust test timeseries frequency: 
			if(c == '+')		test_ts_adjust += 2; 
			if(c == '-')		test_ts_adjust -= 2; 
#endif

			//	temporarily use keystrokes '0'-'8' to switch PIRAQ channel mode on 3 boards
			if((c >= '0') && (c <= '8'))	{	// '0'-'2' piraq1, etc.
				if (c == '0')		{	piraq1->SetCP2PIRAQTestAction(SEND_CHA);	//	send CHA
				printf("piraq1->SetCP2PIRAQTestAction(SEND_CHA)\n");	}
				else if (c == '1')	{	piraq1->SetCP2PIRAQTestAction(SEND_CHB);	//	send CHB 
				printf("piraq1->SetCP2PIRAQTestAction(SEND_CHB)\n");	}
				else if (c == '2')	{	piraq1->SetCP2PIRAQTestAction(SEND_COMBINED);	//	send combined data
				printf("piraq1->SetCP2PIRAQTestAction(SEND_COMBINED)\n");	}
				else if (c == '3')	piraq2->SetCP2PIRAQTestAction(SEND_CHA);	//	send CHA
				else if (c == '4')	piraq2->SetCP2PIRAQTestAction(SEND_CHB);	//	send CHB 
				else if (c == '5')	piraq2->SetCP2PIRAQTestAction(SEND_COMBINED);	//	send combined data
				else if (c == '6')	piraq3->SetCP2PIRAQTestAction(SEND_CHA);	//	send CHA
				else if (c == '7')	piraq3->SetCP2PIRAQTestAction(SEND_CHB);	//	send CHB 
				else if (c == '8')	piraq3->SetCP2PIRAQTestAction(SEND_COMBINED);	//	send combined data
			}
			if(c == 'Q' || c == 27)   {
				printf("\nUser terminated:\n");	
				break;
			}
		} // end if (kbhit())  
	} // end	while(1)

	// exit NOW:
	printf("\npress a key to stop piraqs and exit\n"); while(!keyvalid()) ; 
	printf("piraqs stopped.\nexit.\n\n"); 
	if (piraqs & 0x01) { // slot 1 active
		stop_piraq(config1, piraq1); 
	} 
	if (piraqs & 0x02) { // slot 2 active
		stop_piraq(config2, piraq2); 
	} 
	if (piraqs & 0x04) { // slot 3 active
		stop_piraq(config3, piraq3); 
	} 
	// remove for lab testing: keep transmitter pulses active w/o go.exe running. 12-9-04
	timer_stop(&ext_timer); 
	delete piraq1; 
	delete piraq2; 
	delete piraq3;
	fclose(db_fp);
	//		printf("TimerStartCorrection = %dmSec\n", TimerStartCorrection); 
	exit(0); 

}


