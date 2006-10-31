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
_tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	CP2PIRAQ* piraq1 = 0;
	CP2PIRAQ* piraq2 = 0;
	CP2PIRAQ* piraq3 = 0;

	CONFIG *config1, *config2, *config3;
	char fname1[10]; char fname2[10]; char fname3[10]; // configuration filenames

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
	FILE * dspEXEC; 
	unsigned int bytespergate; 
	__int64 pulsenum, beamnum; 
	time_t now, now_was; 
	int	PIRAQadjustAmplitude = 0; 
	int	PIRAQadjustFrequency = 1; 
	unsigned int julian_day; 
	TIMER ext_timer; // structure defining external timer parameters 
	int iError;


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
	//    Create piraqs. They all have to be created, so that all boards
	//    are found in succesion, even if we will not be collecting data 
	//    from all of them.

	piraq1 = new CP2PIRAQ(config1, Nhits, outport, fname1, argv[1], bytespergate, "/CMD1");
	piraq2 = new CP2PIRAQ(config2, Nhits, outport+1, fname2, argv[1], bytespergate, "/CMD2");
	piraq3 = new CP2PIRAQ(config3, Nhits, outport+2, fname3, argv[1], bytespergate, "/CMD3");

	float prt;
	INFOHEADER info;
	prt = piraq3->prt();
	info = piraq3->info();
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
	printf("pulsenum = %I64d\n", pulsenum); 
	printf("beamnum  =  %I64d\n", beamnum); 

	///////////////////////////////////////////////////////////////////////////
	//
	//      start the piraqs, waiting for each to indicate functionality

	if (piraqs & 0x01) { // turn on slot 1
		if (piraq1->start(pulsenum, beamnum)) 
			exit(-1);
	} 
	if (piraqs & 0x02) { // turn on slot 2
		if (piraq2->start(pulsenum, beamnum)) 
			exit(-1);
	} 
	if (piraqs & 0x04) { // turn on slot 3
		if (piraq3->start(pulsenum, beamnum)) 
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
	printf("pulsenum = %I64d\n", pulsenum); 
	printf("beamnum  =  %I64d\n", beamnum); 

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
	if (piraq1)
		delete piraq1; 
	if (piraq2)
		delete piraq2; 
	if (piraq3)
		delete piraq3;

	fclose(db_fp);
	//		printf("TimerStartCorrection = %dmSec\n", TimerStartCorrection); 
	exit(0); 

}


