#include		<stdafx.h> //  this gets getch() and kbhit() included ... 
#include        <stdio.h>
#include        <ctype.h>
#include        <string.h>
#include        <stdlib.h>
#include        <math.h>
#include		<conio.h>
#include		<windows.h>
#include <winsock2.h>
#include <mmsystem.h>

#include "stdafx.h"      
#include "CP2exec.h" 

#include "../include/proto.h"

#include "get_julian_day.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define COUNTS_PER_DEGREE 182.04444	// move this to proto.h

static	char *FIFONAME = "/PRQDATA";

#define	FIFOADDRESS  0xA00000

//#define			TIME_TESTING		// define to activate millisecond printout for time of events. 
#ifdef CP2_TESTING		// switch ON test code for CP2 
// test drx data throughput limits by varying data packet size w/o changing DSP computational load:  
#define			DRX_PACKET_TESTING	// define to activate data packet resizing for CP2 throughput testing. 
#endif
#define			CYCLE_HITS	20	// #fifo hits to take from piraq-host shared memory before rotating to next board: CP2
//#define NO_INTEGER_BEAMS // for staggered PRT testing, etc., defeat angle interpolation, etc. 

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
using namespace std;

PIRAQ        *piraq;  // allocate pointers for object instantiation
PIRAQ        * piraq1, * piraq2, * piraq3;  // try more 
#ifdef TIMER_PIRAQ
PIRAQ        * timer_piraq; // extra PIRAQ used as substitute for external timer card
#endif
CONFIG       *config;
CONFIG       * config1, * config2, * config3;
FIRFILTER    *FirFilter;

//  pointers to FIR channel allocations
unsigned short * firch1_i; 
unsigned short * firch1_q; 
unsigned short * firch2_i; 
unsigned short * firch2_q; 

#define			ABSCALE		1.E+4
#define			PSCALE		1.0
#define			DBOFFSET	80.0

//for mSec-resolution time tests: 
#include <sys/timeb.h>
#include <time.h>
struct _timeb timebuffer;
char *timeline;

static unsigned short last_millisec, delta_millisec; 

// set/compute #hits combined by piraq: equal in both piraq executable (CP2_DCCS3_1.out) and CP2exec.exe 
unsigned int Nhits; 
unsigned int packets = 0; 

/////////////////////////////////////////////////////////////////////////////
int
init_piraq(PIRAQ* &pPiraq, FIFO* &pCmd, char* cmdFifoName, FIFO* &pFifo, CONFIG* pConfig, 
		   PACKET* &pPkt, int &cmd_notifysock, int bytespergate, char* configFname, char* dspObjFname) 
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
		Nhits * (HEADERSIZE + (config1->gatesa * bytespergate) + BUFFER_EPSILON), 
		PIRAQ_FIFO_NUM); 
	printf("hit size = %d computed Nhits = %d\n", (HEADERSIZE + (config1->gatesa * bytespergate) + BUFFER_EPSILON), Nhits); 

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
	timerset(config1, pPiraq); // !note: also programs pll and FIR filter. 
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
_tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int i,j,k,y,outport;
	int cmd1_notifysock, cmd2_notifysock, cmd3_notifysock; 
	int val1, val2, val3; 
	unsigned __int64 temp1, temp2, temp3; 
	int outsock1, outsock2, outsock3; 
	int piraqnum=1, gates_gg;
	char c,name[80];
	FIFO *fifo1, *fifo2, *fifo3; 
	FIFO *cmd1, *cmd2, *cmd3; 
	UDPHEADER *udp1, *udp2, *udp3;
	struct timeval tv1, tv2, tv3;
	fd_set rfd1, rfd2, rfd3;
	PACKET *fifopiraq1, *fifopiraq2, *fifopiraq3;
	PACKET *pkt1, *pkt2, *pkt3, *pn_pkt; 

	config1	= new CONFIG; 
	config2	= new CONFIG; 
	config3	= new CONFIG; 
	char fname1[10]; char fname2[10]; char fname3[10]; // configuration filenames

	float pcorrect;
	float		az1 = 0, az2 = 0, az3 = 0, el1 = 0, el2 = 0, el3 = 0;
	unsigned int scan1 = 0, scan2 = 0, scan3 = 0, volume1 = 0, volume2 = 0, volume3 = 0; 

	int nRetCode = 0; // used by MFC 
	int testnum = 1;  // console output sequential test number 
	int r_c; // return code 
	int piraqs = 0;   // board count -- default to single board operation 
	int fifo1_hits, fifo2_hits, fifo3_hits; // cumulative hits per board 
	int cur_fifo1_hits, cur_fifo2_hits, cur_fifo3_hits; // current hits per board 
	int cycle_fifo1_hits, cycle_fifo2_hits, cycle_fifo3_hits; // current hits per cycle 
	cycle_fifo1_hits = cycle_fifo2_hits = cycle_fifo3_hits = 0; // clear hits per cycle     
	float *fsrc, *diagiqsrc;
	FILE * dspEXEC; 
	int cmdline_filename = FALSE; 
	int dspl_hits = 100; // fifo hits modulus for updating display 
	char dspl_format = FALSE; 
	unsigned int seq1, seq2, seq3; 
	unsigned int bytespergate; 

	__int64 lastpulsenumber1, lastpulsenumber2, lastpulsenumber3;
	lastpulsenumber1 = lastpulsenumber2 = lastpulsenumber3 = 0;

	__int64 lastbeamnumber1, lastbeamnumber2, lastbeamnumber3;
	lastbeamnumber1 = lastbeamnumber2 = lastbeamnumber3 = 0;

	int  PNerrors1, PNerrors2, PNerrors3; 
	PNerrors1 = PNerrors2 = PNerrors3 = 0; 

	float scale1, scale2, scale3; 
	unsigned int hits, hits1, hits2, hits3; 
	unsigned int gates1, gates2, gates3; 

	unsigned int errors = 0; 
	unsigned int errors1, errors2, errors3; 
	errors1 = errors2 = errors3 = 0;

	int	PIRAQadjustAmplitude = 0; 
	int	PIRAQadjustFrequency = 1; 

	// external timer card:
	TIMER ext_timer; // structure defining external timer parameters 
	unsigned int julian_day; 

	// getsockopt parameters, initialization: 
	int iError;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}

	// open debug data file: 
	db_fp = fopen("debug.dat","w");
	fprintf(db_fp,"CP2exec.exe results:\n");  

	printf("CP2exec: usage -- \n"); 
	printf("         default output localhost\n"); 
	printf("         default outport 3100\n"); 
	printf("         parameters --\n"); 
	printf("         <DSP filename> <received data format: 'f' floats, 's' unsigned shorts>\n"); 
	if (argc > 1) { // entered a filename 
		if ((dspEXEC = fopen(argv[1],"r")) == NULL) // DSP executable file not found 
		{ printf("Usage: %s <DSP filename> DSP executable file not found\n", argv[0]); exit(0); 
		} 
		cmdline_filename = TRUE; 
		fclose(dspEXEC); // existence test passed; use command-line filename
	}
	printf("file %s will be loaded into piraq\n", argv[1]); 
	if (argc > 2) { // entered a format
		dspl_format = toupper(*argv[2]); 
		if (dspl_format == 'F') { // display short data 
			printf("FLOAT format specified for piraq received data.\n"); 
			printf("data displayed every fifo hit\n"); 
			dspl_hits = 10; // was 10 
		} 
		else { // no data display
			printf("NO DISPLAY of piraq received data\n"); 
		} 
	}
	if (argc > 3) { // entered a piraq BOARD-enable mask (0-7)
		piraqs = atoi(argv[3]); 
		printf("piraq select mask = %d\n", piraqs); 
	}
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

	// this if/else wraps around the entire main: else {...} contains single-board operation. 
	if (piraqs) { // entered slot pattern: bit set runs board, right-to-left 
		// (right board most significant bit). 
		// use defaults: outport = 3100 outsock opens "localhost"
		outport = 3100; 
		//outport = 21010; //!CP2Scope direct!
		iError = 0; WSASetLastError(iError);
		iError = WSAGetLastError ();
		printf("set/get iError = %d\n",iError);
		//as it was:	if((outsock = open_udp_out("localhost")) ==  ERROR)			/* open one socket */
		int	send_one = 1;

		// open sockets
		if((outsock1 = open_udp_out("192.168.3.255")) ==  ERROR)			/* open one socket */
		{
			printf("%s: Could not open output socket 1\n",name); 
			exit(0);
		}
		printf("udp socket opens; outsock1 = %d\n", outsock1); 
		if((outsock2 = open_udp_out("192.168.3.255")) ==  ERROR)			/* open second socket */
		{
			printf("%s: Could not open output socket 2\n",name); 
			exit(0);
		}
		printf("udp socket opens; outsock2 = %d\n", outsock2); 
		if((outsock3 = open_udp_out("192.168.3.255")) ==  ERROR)			/* open second socket */
		{
			printf("%s: Could not open output socket 3\n",name); 
			exit(0);
		}
		printf("udp socket opens; outsock3 = %d\n", outsock3); 

		iError = WSAGetLastError ();
		printf("open_udp_out(): iError = %d\n",iError);
		printf("set/get iError = %d\n",iError);

		//eof_start_over: 
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
		else	{	//	no other dataformats supported
			printf("dataformat != 18 not supported\n"); 
			exit(0); 
		}

		///////////////////////////////////////////////////////////////////////////
		//
		//    Initialize piraqs

		if (piraqs & 1) {
			piraq1 = new PIRAQ;
			if (init_piraq(piraq1, cmd1, "/CMD1", fifo1, config1, pkt1, 
				cmd1_notifysock, bytespergate, fname1, argv[1]))
				piraqs &= ~1;
			else 
				pn_pkt = pkt1;
		}

		if (piraqs & 2) {
			piraq2 = new PIRAQ;
			if (init_piraq(piraq2, cmd2, "/CMD2", fifo2, config2, pkt2, 
				cmd2_notifysock, bytespergate, fname2, argv[1]))
				piraqs &= ~2;
			else 
				pn_pkt = pkt2;
		}

		if (piraqs & 4) {
			piraq3 = new PIRAQ;
			if (init_piraq(piraq3, cmd3, "/CMD2", fifo3, config3, pkt3, 
				cmd3_notifysock, bytespergate, fname3, argv[1]))
				piraqs &= ~4;
			else 
				pn_pkt = pkt3;
		}

		///////////////////////////////////////////////////////////////////////////
		//
		//      Don't know what the following stuff is all about

		// next section waits for PPS edge and then starts external timer card
		// get time, then wait for new second
		time_t now, now_was; 
		__int64 pulsenum, beamnum; 
		float prt = pn_pkt->data.info.prt[0];   // get PRTs from valid packet as determined above. 
		float prt2 = pn_pkt->data.info.prt[1]; 

		//		if (pn_pkt->data.info.dataformat == 17) // staggered PRT
		//			prt += prt2; 
		// from singlepiraq now041505.cpp: 
		if (pn_pkt->data.info.dataformat == 17) { // staggered PRT
			prt += prt2; 
			prt /= 2.0; // staggered PRT: two hits per combined prt
			printf("STAGGER: combined prt = %+8.3e\n", prt); 
		}

		__int64 prf; 
		prf = (__int64)(ceil((double)(1/prt)));
		unsigned int pri; 
		printf("pn_pkt->data.info.prt[0] = %f (float)(1/prt) = %+8.3e\n", pn_pkt->data.info.prt[0], (float)(1/prt)); 
		// fp: 
		double fpprf, fpsuppm; 
		fpprf = ((double)1.0)/((double)prt); 
		//!board-specific for 2,3
		hits = config1->hits; 
		fpsuppm = fpprf/(double)hits; 
		//		fpExactmSecperBeam = ((double)1000.0)/fpsuppm; 
		printf("prt = %+8.3e fpprf = %+8.3e fpsuppm = fpprf/hits = %+8.3e\n", prt, fpprf, fpsuppm); 
		//		printf("fpExactmSecperBeam = %8.3e\n", fpExactmSecperBeam); 

		pri = (unsigned int)(((float)COUNTFREQ)/(float)(1/prt)) + 0.5; 
		printf("pri = %d\n", pri); 
		printf("prf = %I64d\n", prf); 
		printf("prt2 = %8.3e\n", pn_pkt->data.info.prt[1]); 
		//!board-specific for 2,3
		hits = config1->hits; 
		printf("hits = %d\n", hits); 
		float suppm; suppm = prf/(float)hits; 
		printf("ceil(prf/hits) = %4.5f, prf/hits = %4.5f\n", ceil(prf/(float)hits), suppm); 
		//		if (ceil(prf/hits) != suppm) {
		//			printf("integral beams/sec required\n"); exit(0); 
		//		} 
#ifdef NO_INTEGER_BEAMS
		goto no_int_beams; 
#endif

		// get current second and wait for it to pass; 
		now = time(&now); now_was = now;
		while(now == now_was) // current second persists 
			now = time(&now);	//  
		now = time(&now); now_was = now;
		printf("now WILL BE %I64d\n", timebuffer.time + (__int64)2); 
		pulsenum = ((((__int64)(now+2)) * (__int64)COUNTFREQ) / pri) + 1; 
		beamnum = pulsenum / hits;
		//beamnum += (__int64)(TimerStartCorrection/mSecperBeam); // cannot do this here ... do at interpolation time
		printf("pulsenum=%I64d\n", pulsenum); 
		printf("beamnum=%I64d\n", beamnum); 


		///////////////////////////////////////////////////////////////////////////
		//
		//      start the piraqs, waiting for each to indicate functionality

		if (piraqs & 0x01) { // turn on slot 1
			if (start_piraq(piraq1, config1, pkt1, pulsenum, beamnum)) 
				exit(-1);
		} 
		if (piraqs & 0x02) { // turn on slot 2
			if (start_piraq(piraq2, config2, pkt2, pulsenum, beamnum)) 
				exit(-1);
		} 
		if (piraqs & 0x04) { // turn on slot 3
			if (start_piraq(piraq3, config3, pkt3, pulsenum, beamnum)) 
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
		beamnum = pulsenum / hits;  
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
		start_timer_card(&ext_timer, &pn_pkt->data.info); // pn_pkt = PACKET pointer to a live piraq 

		// all running -- get data!
		testnum = 0;  fifo1_hits = 0; fifo2_hits = 0; fifo3_hits = 0; // 
		seq1 = seq2 = seq3 = 0; // initialize sequence# for each channel 
		while(1) { // until 'q' 
			julian_day = get_julian_day(); 
			if (piraqs & 0x01) { // turn on slot 1
				tv1.tv_sec = 0;
				tv1.tv_usec = 1000; 
				/* do polling */
				FD_ZERO(&rfd1);
				FD_SET(cmd1_notifysock, &rfd1); //!!!Mitch eliminated but while(1) below depends on it. 
				val1 = select(cmd1_notifysock + 1, &rfd1, 0, 0, &tv1);
				//val1 = 0; // force it
				//val1 = select(cmd1_notifysock, &rfd1, 0, 0, &tv1); // eliminate + 1
				if (val1 < 0) 
				{printf( "select1 val = 0x%x\n", val1 );  continue;}

				else if(val1 == 0) { /* use time out for polling */
					// PIRAQ1:
					// take CYCLE_HITS beams from piraq:
					while(((cur_fifo1_hits = fifo_hit(fifo1)) > 0) && (cycle_fifo1_hits < CYCLE_HITS)) { // fifo1 hits ready: save #hits pending 
						//if ((cur_fifo1_hits % 5) == 0) { printf("b0: hits1 = %d\n", cur_fifo1_hits); } // print often enough ... NCAR-Radar-DRX
						//if (((cur_fifo1_hits % 2) == 0) || (cur_fifo1_hits < 2)) { printf("b0: hits1 = %d\n", cur_fifo1_hits); } // print often enough ... on atd-milan
						cycle_fifo1_hits++; 
						fifopiraq1 = (PACKET *)fifo_get_read_address(fifo1,0); 
#ifdef EOF_DETECT
						if ((int)fifopiraq1->data.info.packetflag == -1) { // piraq detected a hardware out-of-sync condition "EOF" 
							printf("\n\npiraq1 EOF detected. exiting.\n"); 
							if (piraqs & 0x01) { // slot 1 active
								stop_piraq(config1, piraq1); 
							} 
							printf("\n\npiraq1 stopped\n"); 
							if (piraqs & 0x02) { // slot 2 active
								stop_piraq(config2, piraq2); 
							} 
							if (piraqs & 0x04) { // slot 3 active
								stop_piraq(config3, piraq3); 
							} 
							printf("\n\npiraqs stopped\n"); 
							timer_stop(&ext_timer); 
							delete piraq1; delete piraq2; delete piraq3; 
							printf("\n\ngoing to eof_start_over\n"); 
							goto eof_start_over; 
						}
#endif // end #ifdef EOF_DETECT
						hits1 = fifopiraq1->data.info.hits; gates1 = fifopiraq1->data.info.gates; 
						scale1 = (float)(PIRAQ3D_SCALE*PIRAQ3D_SCALE*hits1); // scale fifo1 data 
						udp1 = &fifopiraq1->udp;
						udp1->magic = MAGIC;
						udp1->type = UDPTYPE_PIRAQ_CP2_TIMESERIES; 
						fifopiraq1->data.info.bytespergate = 2 * (sizeof(float)); // Staggered PRT ABPDATA
						fsrc = (float *)fifopiraq1->data.data; 
						// no data scaling in CP2.exe

						temp1 = fifopiraq1->data.info.pulse_num * (unsigned __int64)(prt * (float)COUNTFREQ + 0.5);
						fifopiraq1->data.info.secs = temp1 / COUNTFREQ;
						fifopiraq1->data.info.nanosecs = ((unsigned __int64)10000 * (temp1 % ((unsigned __int64)COUNTFREQ))) / (unsigned __int64)COUNTFREQ;
						fifopiraq1->data.info.nanosecs *= (unsigned __int64)100000; // multiply by 10**5 to get nanoseconds
						az1 += 0.5;  
						if(az1 > 360.0) { // full scan 
							az1 -= 360.0; // restart azimuth angle 
							el1 += 7.0;		// step antenna up 
							scan1++; // increment scan count
							if (el1 >= 21.0) {	// beyond allowed step
								el1 = 0.0; // start over at horizon
								volume1++; // finish volume
							}
						} 
						fifopiraq1->data.info.az = az1;  
						fifopiraq1->data.info.el = el1; // set in packet 
						fifopiraq1->data.info.scan_num = scan1;
						fifopiraq1->data.info.vol_num = volume1;  
						//					printf("fake: az = %4.3f el = %4.3f\n",fifopiraq1->data.info.az,fifopiraq1->data.info.el); 
						// ... to here 
#ifdef TIME_TESTING	
						// for mSec-resolution time tests: 
						_ftime( &timebuffer );
						timeline = ctime( & ( timebuffer.time ) );
						printf( "1: %hu ... fakin it.\n", timebuffer.millitm );
#endif
						fifopiraq1->data.info.julian_day = julian_day; 

#ifdef	DRX_PACKET_TESTING	// define to activate data packet resizing for throughput testing. 
						//					fifopiraq1->data.info.recordlen = (fifopiraq1->data.info.clutter_end[0])*fifopiraq1->data.info.gates*24 + (RECORDLEN(fifopiraq1)); // vary multiplier; note CPU usage in Task Manager. 24 = 2*bytespergate. 
						fifopiraq1->data.info.recordlen = Nhits*(RECORDLEN(fifopiraq1)+BUFFER_EPSILON); /* this after numgates corrected */
#else
						fifopiraq1->data.info.recordlen = RECORDLEN(fifopiraq1); /* this after numgates corrected */
#endif
						// scale data -- offload from piraq: 
						fsrc = (float *)fifopiraq1->data.data; 

						__int64 * __int64_ptr, * __int64_ptr2; unsigned int * uint_ptr; float * fsrc2; 
						for (i = 0; i < Nhits; i++) { // all hits in the packet 
							// compute pointer to datum in an individual hit, dereference and print. 
							// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config1->gatesa * bytespergate) + BUFFER_EPSILON)
							__int64_ptr = (__int64 *)((char *)&fifopiraq1->data.info.beam_num + i*((HEADERSIZE + (config1->gatesa * bytespergate) + BUFFER_EPSILON))); 
							beamnum = *__int64_ptr; 
							__int64_ptr2 = (__int64 *)((char *)&fifopiraq1->data.info.pulse_num + i*((HEADERSIZE + (config1->gatesa * bytespergate) + BUFFER_EPSILON))); 
							pulsenum = *__int64_ptr2; 
							uint_ptr = (unsigned int *)((char *)&fifopiraq1->data.info.hits + i*((HEADERSIZE + (config1->gatesa * bytespergate) + BUFFER_EPSILON))); 
							j = *uint_ptr; 
							uint_ptr = (unsigned int *)((char *)&fifopiraq1->data.info.channel + i*((HEADERSIZE + (config1->gatesa * bytespergate) + BUFFER_EPSILON))); 
							k = *uint_ptr; 
							fsrc2 = (float *)((char *)fifopiraq1->data.data + i*((HEADERSIZE + (config1->gatesa * bytespergate) + BUFFER_EPSILON)));
							//printf("hit%d: %+8.3e %+8.3e %+8.3e %+8.3e \n", i, *(fsrc2+0), *(fsrc2+1), *(fsrc2+2), *(fsrc2+3)); 
							//printf("hit%d: %+8.6e %+8.6e %+8.3e %+8.3e \n", i, *(fsrc2+0), *(fsrc2+1), *(fsrc2+2), *(fsrc2+3)); 
							//printf("hit%d: %+8.3e %+8.3e %+8.3e %+8.3e \n", i, *(fsrc2+4), *(fsrc2+5), *(fsrc2+6), *(fsrc2+7)); 
							//printf("hit%d: %+8.3e\n", i, *(fsrc2+0)); 
							//printf("hit%d: %d\n", i, fifopiraq1->cmd.arg[1]); 
							//if (i < 2) { printf("hit%d: %+8.3e\n", i, *(fsrc2+0)); }	//	reduced printing

							if (lastpulsenumber1 != pulsenum - 1) { // PNs not sequential
								printf("hit%d: lastPN = %I64d PN = %I64d\n", i+1, lastpulsenumber1, pulsenum);  PNerrors1++; 
								fprintf(db_fp, "%d:hit%d: lastPN = %I64d PN = %I64d\n", fifopiraq1->data.info.channel, i+1, lastpulsenumber1, pulsenum); 
							}
							lastpulsenumber1 = pulsenum; // previous hit PN
						}
						if ((testnum % dspl_hits) == 0) { // done dspl_hit cycles
							if (dspl_format == 'F') { // display short data
								pcorrect = 1.0; //(float)fifopiraq1->data.info.hits; //pow(10., 0.1*fifopiraq1->data.info.data_sys_sat);
								fsrc = (float *)fifopiraq1->data.data;
								gates_gg = (int)fifopiraq1->data.info.gates*6-6;
							}
						} // end	if ((testnum % ...
						testnum++; fifo1_hits++;   
#ifndef DRX_PACKET_TESTING	// define activates data packet resizing for CP2 throughput testing. 
						fifopiraq1->udp.totalsize = TOTALSIZE(fifopiraq1); // ordinary operation
#else	// increase udp-send totalsize to Nhits
						int test_totalsize1 = Nhits*(TOTALSIZE(fifopiraq1) + BUFFER_EPSILON); 
						//works ... int test_totalsize1 = Nhits*(TOTALSIZE(fifopiraq1) + BUFFER_EPSILON) + 6*sizeof(UDPHEADER); // add Nhits-1 udpsend adds 1 more ... fuckinay, man
						fifopiraq1->udp.totalsize = test_totalsize1; // CP2 throughput testing
						packets++; 
						if	(packets == 10)	{printf("packet totalsize1 %d\n", test_totalsize1);}
#endif

						seq1 = send_udp_packet(outsock1, outport, seq1, udp1); 
						fifo_increment_tail(fifo1);
					} // end	while(fifo_hit()
					cycle_fifo1_hits = 0; // clear cycle counter 
				} // end	else if(val == 0) 
			}
			// piraq2: 
			if (piraqs & 0x02) { // turn on slot 2
				tv2.tv_sec = 0;
				tv2.tv_usec = 1000; 
				/* do polling */
				FD_ZERO(&rfd2);
				FD_SET(cmd2_notifysock, &rfd2); //!!!Mitch eliminated but while(1) below depends on it.  
				val2 = select(cmd2_notifysock + 1, &rfd2, 0, 0, &tv2);
				if (val2 < 0) 
				{perror( "select" );  continue;}

				else if(val2 == 0) { /* use time out for polling */
					// PIRAQ2:
					// take CYCLE_HITS beams from piraq:
					while(((cur_fifo2_hits = fifo_hit(fifo2)) > 0) && (cycle_fifo2_hits < CYCLE_HITS)) { // fifo1 hits ready: save #hits pending 
						if ((cur_fifo2_hits % 5) == 0) { printf("b1: hits2 = %d\n", cur_fifo2_hits); } // print often enough ... 
						cycle_fifo2_hits++; 
						fifopiraq2 = (PACKET *)fifo_get_read_address(fifo2,0);
#ifdef EOF_DETECT
						if ((int)fifopiraq2->data.info.packetflag == -1) { // piraq detected a hardware out-of-sync condition "EOF" 
							printf("\n\npiraq2 EOF detected. exiting.\n"); 
							if (piraqs & 0x01) { // slot 1 active
								stop_piraq(config1, piraq1); 
							} 
							printf("\n\npiraq1 stopped\n"); 
							if (piraqs & 0x02) { // slot 2 active
								stop_piraq(config2, piraq2); 
							} 
							if (piraqs & 0x04) { // slot 3 active
								stop_piraq(config3, piraq3); 
							} 
							printf("\n\npiraqs stopped\n"); 
							timer_stop(&ext_timer); 
							delete piraq1; delete piraq2; delete piraq3; 
							printf("\n\ngoing to eof_start_over\n"); 
							goto eof_start_over; 
						}
#endif // end #ifdef EOF_DETECT
						hits2 = fifopiraq2->data.info.hits; gates2 = fifopiraq2->data.info.gates; 
						scale2 = (float)(PIRAQ3D_SCALE*PIRAQ3D_SCALE*hits2); // scale fifo2 data
						udp2 = &fifopiraq2->udp;
						udp2->magic = MAGIC;
						udp2->type = UDPTYPE_PIRAQ_CP2_TIMESERIES; 
						fifopiraq2->data.info.bytespergate = 2 * (sizeof(float)); // Staggered PRT ABPDATA
						fsrc = (float *)fifopiraq2->data.data; 
						// no data scaling
						temp2 = fifopiraq2->data.info.pulse_num * (unsigned __int64)(prt * (float)COUNTFREQ + 0.5);
						fifopiraq2->data.info.secs = temp2 / COUNTFREQ;
						fifopiraq2->data.info.nanosecs = ((unsigned __int64)10000 * (temp2 % ((unsigned __int64)COUNTFREQ))) / (unsigned __int64)COUNTFREQ;
						fifopiraq2->data.info.nanosecs *= (unsigned __int64)100000; // multiply by 10**5 to get nanoseconds

						az2 += 0.5;  
						if(az2 > 360.0) { // full scan 
							az2 -= 360.0; // restart azimuth angle 
							el2 += 7.0;		// step antenna up 
							scan2++; // increment scan count
							if (el2 >= 21.0) {	// beyond allowed step
								el2 = 0.0; // start over at horizon
								volume2++; // finish volume
							}
						} 
						fifopiraq2->data.info.az = az2;  
						fifopiraq2->data.info.el = el2; // set in packet 
						fifopiraq2->data.info.scan_num = scan2;
						fifopiraq2->data.info.vol_num = volume2;
						// ... to here 
#ifdef TIME_TESTING	
						// for mSec-resolution time tests: 
						_ftime( &timebuffer );
						timeline = ctime( & ( timebuffer.time ) );
						printf( "2: %hu ... fakin it.\n", timebuffer.millitm );
#endif
						fifopiraq2->data.info.julian_day = julian_day; 

#ifdef	DRX_PACKET_TESTING	// define to activate data packet resizing for throughput testing. 
						//					fifopiraq2->data.info.recordlen = (fifopiraq2->data.info.clutter_end[0])*fifopiraq2->data.info.gates*24 + (RECORDLEN(fifopiraq2)); // vary multiplier; note CPU usage in Task Manager. 24 = 2*bytespergate. 
						fifopiraq2->data.info.recordlen = Nhits*(RECORDLEN(fifopiraq2)+BUFFER_EPSILON); /* this after numgates corrected */
#else
						fifopiraq2->data.info.recordlen = RECORDLEN(fifopiraq2); /* this after numgates corrected */
#endif

						fsrc = (float *)fifopiraq2->data.data; 
						__int64 * __int64_ptr, * __int64_ptr2; unsigned int * uint_ptr; float * fsrc2; 
						for (i = 0; i < Nhits; i++) { // all hits in the packet 
							// compute pointer to datum in an individual hit, dereference and print. 
							// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config2->gatesa * bytespergate) + BUFFER_EPSILON)
							__int64_ptr = (__int64 *)((char *)&fifopiraq2->data.info.beam_num + i*((HEADERSIZE + (config2->gatesa * bytespergate) + BUFFER_EPSILON))); 
							beamnum = *__int64_ptr; 
							__int64_ptr2 = (__int64 *)((char *)&fifopiraq2->data.info.pulse_num + i*((HEADERSIZE + (config2->gatesa * bytespergate) + BUFFER_EPSILON))); 
							pulsenum = *__int64_ptr2; 
							uint_ptr = (unsigned int *)((char *)&fifopiraq2->data.info.hits + i*((HEADERSIZE + (config2->gatesa * bytespergate) + BUFFER_EPSILON))); 
							j = *uint_ptr; 
							uint_ptr = (unsigned int *)((char *)&fifopiraq2->data.info.channel + i*((HEADERSIZE + (config2->gatesa * bytespergate) + BUFFER_EPSILON))); 
							k = *uint_ptr; 
							fsrc2 = (float *)((char *)fifopiraq2->data.data + i*((HEADERSIZE + (config2->gatesa * bytespergate) + BUFFER_EPSILON)));
							fsrc2 += 0; // look at THIS datum
							if (lastpulsenumber2 != pulsenum - 1) { // PNs not sequential
								printf("hit%d: lastPN = %I64d PN = %I64d\n", i+1, lastpulsenumber2, pulsenum);  PNerrors2++; 
								fprintf(db_fp, "%d:hit%d: lastPN = %I64d PN = %I64d\n", fifopiraq2->data.info.channel, i+1, lastpulsenumber2, pulsenum); 
							} 
							lastpulsenumber2 = pulsenum; 
							//					printf("hit%d: BN = %I64d PN = %I64d hits = %d gates = %d fsrc2[6] = %+8.3f\n", i+1, beamnum, pulsenum, j, k, *fsrc2); 
							//					printf("%d: hit%02d: BN = %I64d PN = %I64d fsrc2[0] = %+8.3f fsrc2[1] = %+8.3f fsrc2[2] = %+8.3f fsrc2[end] = %+8.3f\n", k, i+1, beamnum, pulsenum, *fsrc2, fsrc2[1], fsrc2[2], fsrc2[(2*fifopiraq2->data.info.gates)-1]); 
						}
						//				printf("\n"); 
						if ((testnum % dspl_hits) == 0) { // done dspl_hit cycles
							if (dspl_format == 'F') { // display short data 
								fsrc = (float *)fifopiraq2->data.data;
								gates_gg = (int)fifopiraq2->data.info.gates*6-6;

								if (config2->ts_start_gate >= 0) { 
									diagiqsrc = (float *)(((unsigned char *)fifopiraq2->data.data) + fifopiraq2->data.info.gates*fifopiraq2->data.info.bytespergate*sizeof(float));  
								}
							} 
						} // end	if ((testnum % ...
						testnum++; fifo2_hits++;  
#ifndef DRX_PACKET_TESTING	// define activates data packet resizing for CP2 throughput testing. 
						fifopiraq2->udp.totalsize = TOTALSIZE(fifopiraq2); // ordinary operation
#else	// increase udp-send totalsize to Nhits
						int test_totalsize2 = Nhits*(TOTALSIZE(fifopiraq2) + BUFFER_EPSILON); // 
						fifopiraq2->udp.totalsize = test_totalsize2; // CP2 throughput testing
#endif

						seq2 = send_udp_packet(outsock2, outport + 1, seq2, udp2);  
						fifo_increment_tail(fifo2);
					} // end	while(fifo_hit()
					cycle_fifo2_hits = 0; 
				} // end	else if(val == 0) 
			}// end piraq2	
			// piraq3: 
			if (piraqs & 0x04) { // turn on slot 3
				tv3.tv_sec = 0;
				tv3.tv_usec = 1000; 
				/* do polling */
				FD_ZERO(&rfd3);
				FD_SET(cmd3_notifysock, &rfd3); //!!!Mitch eliminated but while(1) below depends on it.  
				val3 = select(cmd3_notifysock + 1, &rfd3, 0, 0, &tv3);
				if (val3 < 0) 
				{perror( "select" );  continue;}

				else if(val3 == 0) { /* use time out for polling */
					// PIRAQ3: 
					// take CYCLE_HITS beams from piraq:
					while(((cur_fifo3_hits = fifo_hit(fifo3)) > 0) && (cycle_fifo3_hits < CYCLE_HITS)) { // fifo1 hits ready: save #hits pending 
						if ((cur_fifo3_hits % 5) == 0) { printf("b2: hits3 = %d\n", cur_fifo3_hits); } // print often enough ... 
						cycle_fifo3_hits++; 
						fifopiraq3 = (PACKET *)fifo_get_read_address(fifo3,0);
#ifdef EOF_DETECT
						if ((int)fifopiraq3->data.info.packetflag == -1) { // piraq detected a hardware out-of-sync condition "EOF" 
							printf("\n\npiraq3 EOF detected. exiting.\n"); 
							if (piraqs & 0x01) { // slot 1 active
								stop_piraq(config1, piraq1); 
							} 
							printf("\n\npiraq1 stopped\n"); 
							if (piraqs & 0x02) { // slot 2 active
								stop_piraq(config2, piraq2); 
							} 
							if (piraqs & 0x04) { // slot 3 active
								stop_piraq(config3, piraq3); 
							} 
							printf("\n\npiraqs stopped\n"); 
							timer_stop(&ext_timer); 
							delete piraq1; delete piraq2; delete piraq3; 
							printf("\n\ngoing to eof_start_over\n"); 
							goto eof_start_over; 
						}
#endif // end #ifdef EOF_DETECT
						hits3 = fifopiraq3->data.info.hits; gates3 = fifopiraq3->data.info.gates; 
						scale3 = (float)(PIRAQ3D_SCALE*PIRAQ3D_SCALE*hits3); // scale fifo3 data
						udp3 = &fifopiraq3->udp;
						udp3->magic = MAGIC;
						udp3->type = UDPTYPE_PIRAQ_CP2_TIMESERIES; 
						fifopiraq3->data.info.bytespergate = 2 * (sizeof(float)); // Staggered PRT ABPDATA
						fsrc = (float *)fifopiraq3->data.data; 
						// no data scaling
						temp3 = fifopiraq3->data.info.pulse_num * (unsigned __int64)(prt * (float)COUNTFREQ + 0.5);
						fifopiraq3->data.info.secs = temp3 / COUNTFREQ;
						fifopiraq3->data.info.nanosecs = (10000 * (temp3 % COUNTFREQ)) / COUNTFREQ;
						fifopiraq3->data.info.nanosecs = ((unsigned __int64)10000 * (temp3 % ((unsigned __int64)COUNTFREQ))) / (unsigned __int64)COUNTFREQ;
						fifopiraq3->data.info.nanosecs *= (unsigned __int64)100000; // multiply by 10**5 to get nanoseconds

						az3 += 0.5;  
						if(az3 > 360.0) { // full scan 
							az3 -= 360.0; // restart azimuth angle 
							el3 += 7.0;		// step antenna up 
							scan3++; // increment scan count
							if (el3 >= 21.0) {	// beyond allowed step
								el3 = 0.0; // start over at horizon
								volume3++; // finish volume
							}
						}
						fifopiraq3->data.info.az = az3;  
						fifopiraq3->data.info.el = el3; // set in packet 
						fifopiraq3->data.info.scan_num = scan3;
						fifopiraq3->data.info.vol_num = volume3; 
						// ... to here 
#ifdef TIME_TESTING	
						// for mSec-resolution time tests: 
						_ftime( &timebuffer );
						timeline = ctime( & ( timebuffer.time ) );
						printf( "3: %hu ... fakin it.\n", timebuffer.millitm );
#endif
						fifopiraq3->data.info.julian_day = julian_day; 
#ifdef	DRX_PACKET_TESTING	// define to activate data packet resizing for throughput testing. 
						//					fifopiraq3->data.info.recordlen = (fifopiraq3->data.info.clutter_end[0])*fifopiraq3->data.info.gates*24 + (RECORDLEN(fifopiraq3)); // vary multiplier; note CPU usage in Task Manager. 24 = 2*bytespergate. 
						fifopiraq3->data.info.recordlen = Nhits*(RECORDLEN(fifopiraq3) + BUFFER_EPSILON); /* this after numgates corrected */
#else
						fifopiraq3->data.info.recordlen = RECORDLEN(fifopiraq3); /* this after numgates corrected */
#endif

						//!					fifopiraq3->data.info.recordlen = RECORDLEN(fifopiraq3); /* this after numgates corrected */
						fsrc = (float *)fifopiraq3->data.data; 
						__int64 * __int64_ptr, * __int64_ptr2; unsigned int * uint_ptr; float * fsrc2; 
						for (i = 0; i < Nhits; i++) { // all hits in the packet 
							// compute pointer to datum in an individual hit, dereference and print. 
							// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config3->gatesa * bytespergate) + BUFFER_EPSILON)
							__int64_ptr = (__int64 *)((char *)&fifopiraq3->data.info.beam_num + i*((HEADERSIZE + (config3->gatesa * bytespergate) + BUFFER_EPSILON))); 
							beamnum = *__int64_ptr; 
							__int64_ptr2 = (__int64 *)((char *)&fifopiraq3->data.info.pulse_num + i*((HEADERSIZE + (config3->gatesa * bytespergate) + BUFFER_EPSILON))); 
							pulsenum = *__int64_ptr2; 
							uint_ptr = (unsigned int *)((char *)&fifopiraq3->data.info.hits + i*((HEADERSIZE + (config3->gatesa * bytespergate) + BUFFER_EPSILON))); 
							j = *uint_ptr; 
							uint_ptr = (unsigned int *)((char *)&fifopiraq3->data.info.channel + i*((HEADERSIZE + (config3->gatesa * bytespergate) + BUFFER_EPSILON))); 
							k = *uint_ptr; 
							fsrc2 = (float *)((char *)fifopiraq3->data.data + i*((HEADERSIZE + (config3->gatesa * bytespergate) + BUFFER_EPSILON)));
							fsrc2 += 0; // look at THIS datum
							if (lastpulsenumber3 != pulsenum - 1) { // PNs not sequential
								printf("hit%d: lastPN = %I64d PN = %I64d\n", i+1, lastpulsenumber3, pulsenum); PNerrors3++; 
								fprintf(db_fp, "%d:hit%d: lastPN = %I64d PN = %I64d\n", fifopiraq3->data.info.channel, i+1, lastpulsenumber3, pulsenum); 
							} 
							lastpulsenumber3 = pulsenum; // previous hit PN
						}
						//				printf("\n"); 
						if ((testnum % dspl_hits) == 0) { // done dspl_hit cycles
							if (dspl_format == 'F') { // display short data 
								fsrc = (float *)fifopiraq3->data.data;
								gates_gg = (int)fifopiraq3->data.info.gates*6-6;
								//printf("2:TOTALSIZE = %d testnum = %d\n", TOTALSIZE(fifopiraq3), testnum); 
								//printf("                 az = %4.3f el = %4.3f\n",fifopiraq3->data.info.az,fifopiraq3->data.info.el); 
								//					        printf("Gate %04d:A0 = %+8.3e B0 = %+8.3e P0 = %+8.3e \n          A1 = %+8.3e B1 = %+8.3e P1 = %+8.3e\n",
								//					  		  0, fsrc[0], fsrc[1], fsrc[2], fsrc[3], fsrc[4], fsrc[5]); 
								if (config3->ts_start_gate >= 0) { 
									diagiqsrc = (float *)(((unsigned char *)fifopiraq3->data.data) + fifopiraq3->data.info.gates*fifopiraq3->data.info.bytespergate*sizeof(float));  
								}
							} 
						} // end	if ((testnum % ...
						//!!!					printf("fifo3_hits = %d\n",fifo3_hits); 
						testnum++; fifo3_hits++;  
#ifndef DRX_PACKET_TESTING	// define activates data packet resizing for CP2 throughput testing. 
						fifopiraq3->udp.totalsize = TOTALSIZE(fifopiraq3); // ordinary operation
#else	// increase udp-send totalsize to Nhits
						int test_totalsize3 = Nhits*(TOTALSIZE(fifopiraq3) + BUFFER_EPSILON); 
						fifopiraq3->udp.totalsize = test_totalsize3; // CP2 throughput testing
#endif
						seq3 = send_udp_packet(outsock3, outport + 2, seq3, udp3);  
						fifo_increment_tail(fifo3);
					} // end	while(fifo_hit()
					cycle_fifo3_hits = 0; 
				} // end	else if(val == 0) 
			} // end piraq3	
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
				if(c == 'Q' || c == 27)   {printf("sent %d packets, total errors 0:%d 1:%d 2:%d\n", packets, PNerrors1, PNerrors2, PNerrors3 ); printf("\nUser terminated:\n");	break;}
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

	} // end if (piraqs)
	stop_piraq(config1, piraq);	//?
	timer_stop(&ext_timer); 
	delete piraq; 
	printf("\n%d: fifo_hits = %d\n", testnum, testnum); 
	exit(0); 
}

