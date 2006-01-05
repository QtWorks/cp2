// CP2exec.cpp : Defines the entry point for the console application.
//

// 6-23-05 CP2.cpp development tip.

// 4-13-05 source from CSWR with fp angle interpolation. 
// 4-18-05 staggered PRT initial beamnumber correct from singlepiraq now041505.cpp. 

// 1-4-06  renamed to CP2exec.cpp; all files CP2exec.* supercede CP2.* 

#include		<stdafx.h> //  this gets getch() and kbhit() included ... 
#include        <stdio.h>
#include        <ctype.h>
#include        <string.h>
#include        <stdlib.h>
#include        <math.h>
#include		<conio.h>
#include		<windows.h>
//#include <winsock.h>
#include <winsock2.h>
#include <mmsystem.h>

#include "stdafx.h"      
#include "CP2.h" 

#include "../include/proto.h"

#include "get_julian_day.h"

#include "../PMAC/pmac_lib.h"  

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define COUNTS_PER_DEGREE 182.04444	// move this to proto.h

static	char *FIFONAME = "/PRQDATA";

#define	FIFOADDRESS  0xA00000

FIFO *open_piraq_fifo(char *name, int headersize, int recordsize, int recordnum);
void convert(PACKET *pkt); 

int  pmac_WD6_acquire(PACKET * pkt, int interpolate);

void shortmembit(unsigned short addr[],int size);
void shortdatabit(unsigned short which,int size);
void printbits(int num);

#define			DSPNUM		1

#define			GG_DEBUG	1		// Set to non-zero to activate last gate printout
//#define			TIME_TESTING		// define to activate millisecond printout for time of events. 
#ifdef CP2_TESTING		// switch ON test code for CP2 
// test drx data throughput limits by varying data packet size w/o changing DSP computational load:  
#define			DRX_PACKET_TESTING	// define to activate data packet resizing for CP2 throughput testing. 
#endif
//#define			PMAC_SEARCH	// enable search for PMAC physical base address
//#define			EOF_DETECT	// enable packet-level EOF testing. stop/restart data acquisition
//#define PMAC_DLL				// access PMAC via DeltaTau DLL
//#define PMAC_WD6				// access PMAC via Windriver6 -- switch not implemented at present.   
//#define			CYCLE_HITS	10	// #hits to take from piraq before rotating to next board: RapidDOW 
#define			CYCLE_HITS	20	// #fifo hits to take from piraq before rotating to next board: CP2
//#define NO_INTEGER_BEAMS // for staggered PRT testing, etc., defeat angle interpolation, etc. 

//#define TIMER_CARD_TESTING // special executable to program timer board, then exit. 
//#define DC_OFFSET_TESTING    // switch off transmitter OFF/ON messages for dc offset testing
//#define TESTING_TIMESERIES // compute test diagnostic timeseries data in one of two piraq channels: 
//#define TESTING_TIMESERIES_RANGE	// test dynamic reassignment of timeseries start, end gate using 'U','D'
#define	RUNTIME_PACKET_SIZING	// CP2: data packets sized at runtime.

#ifdef		DRX_PACKET_TESTING	// define to activate data packet resizing for CP2 throughput testing. 		
#define PIRAQ3D_SCALE	1.0/pow(2,31)	// normalize 2^31 data to 1.0 full scale using multiplication
#else
#define PIRAQ3D_SCALE	pow(2,31)	// normalize 2^31 data to 1.0 full scale
#endif
// old-timey stuff to help get work done ... 

FILE * db_fp; // debug data file 

int keyvalid() 
{
	while(!kbhit()) { } // no keystroke 
	getch();    // get it
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;

// extra PIRAQ used as 48MHz piraq timing source; requires 10MHz reference input 
//#define TIMER_PIRAQ
//#ifndef NCAR_DRX	// REMOVE: NCAR DRX now has a timer card
//OFF: #define TIMER_CARD
//#endif

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

// PMAC allocations
static PMAC_HANDLE pmac; 
static unsigned short * PMACDPRAM = 0;       // for reading PMAC 
static unsigned short millisec; 
static unsigned short last_millisec, delta_millisec; 
static unsigned short PMAC_acquire_times = 0; 

unsigned int BeamsperSec; 
unsigned int mSecperBeam; 
float ExactmSecperBeam; 
__int64 RadarEpochMillisec; 
__int64 SystemEpochMillisec; 
__int64 TimerStartCorrection; // runtime measurement of timer card startup interval in mSec to apply to REMSEc
// fp: 
double fpExactmSecperBeam; 
double fpRadarMillisec; 
double fpSystemMillisec; 
double fpRadarSystemCorrection;
__int64 RadarMillisec; 
__int64 SystemMillisec; 

// set/compute #hits combined by piraq: equal in both piraq executable (CP2_DCCS3_1.out) and CP2.exe 
//unsigned int Nhits = 4;
unsigned int Nhits; 

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
    int i,j,k,y,outsock,outport,cmdnotifysock,val,clients,*iptr,used;
	int idx_tts = 0; // index for test timeseries data
	int cmd1_notifysock, cmd2_notifysock, cmd3_notifysock; 
	int val1, val2, val3; 
	unsigned __int64 temp1, temp2, temp3; 
	int outsock1, outsock2, outsock3; 
	int outport1, outport2, outport3; 
    int piraqnum=1, gates_gg;
    unsigned __int64 temp; 
    char *ptr,c,name[80];
    FIFO	*cmd,*fifo;
	FIFO	* fifo1, * fifo2, * fifo3; 
	FIFO * cmd1, * cmd2, * cmd3; 
    FIFORING	cmdrcv;
    UDPHEADER	*udp;
    UDPHEADER	* udp1, *udp2, *udp3;
    struct timeval tv;
    struct timeval tv1, tv2, tv3;
    fd_set rfd;
    fd_set rfd1, rfd2, rfd3;
    INFOHEADER	best;
    PACKET		*fifocmd,*iqheader;
    PACKET		*fifopiraq;
	PACKET * lastfifopiraq; 
    PACKET		* fifopiraq1, * fifopiraq2, * fifopiraq3;
    PACKET		*fifoiqdata,*pkt;
	PACKET * pkt1, * pkt2, * pkt3, * pn_pkt; 
    config  = new CONFIG;

    float		az = 0, el = 0, pcorrect;
	unsigned int scan = 0, volume = 0; 
    float		az1 = 0, az2 = 0, az3 = 0, el1 = 0, el2 = 0, el3 = 0;
	float		test_ts_power = 0.1; // multiplier for test timeseries data 
	unsigned int scan1 = 0, scan2 = 0, scan3 = 0, volume1 = 0, volume2 = 0, volume3 = 0; 
    int			test_ts_adjust = 2; // note if 0 test timeseries single frequency gets flat data! 
	int			cnt;
#ifdef NCAR_DRX // for staggered PRT testing, etc., defeat angle interpolation, etc. 
    int			fakeangles = TRUE;	// NCAR_DRX no PMAC so fake it 
#else
    int			fakeangles = FALSE; // get angles, etc., from PMAC 
#endif
    unsigned __int64	oldbnum=0; 
    int			send=0;

	int nRetCode = 0; // used by MFC 
    int testnum = 1;  // console output sequential test number 
    int r_c; // return code 
	int nodefault = FALSE; 
	int piraqs = 0;   // board count -- default to single board operation 
	int fifo_hits, fifo1_hits, fifo2_hits, fifo3_hits; // cumulative hits per board 
	int cur_fifo_hits, cur_fifo1_hits, cur_fifo2_hits, cur_fifo3_hits; // current hits per board 
	int cycle_fifo1_hits, cycle_fifo2_hits, cycle_fifo3_hits; // current hits per cycle 
	cycle_fifo1_hits = cycle_fifo2_hits = cycle_fifo3_hits = 0; // clear hits per cycle     
	int x = 0; unsigned short FIRCH_1values[256]; unsigned int FIRCount = 32; 
    unsigned short * ssrc, * testiqsrc; 
    float * fsrc, * diagiqsrc;
	float * fsrc2; 
	float	A0, B0, P0, A1, B1, P1;
	char Cdisp[132];
	FILE * dspEXEC; 
	int cmdline_filename = FALSE; 
	int dspl_hits = 100; // fifo hits modulus for updating display 
	char dspl_format = FALSE; 
	unsigned int seq, seq1, seq2, seq3; 
	unsigned int bytespergate; 

	__int64 lastpulsenumber = 0;
	__int64 lastpulsenumber1, lastpulsenumber2, lastpulsenumber3;
	lastpulsenumber1 = lastpulsenumber2 = lastpulsenumber3 = 0;
	__int64 lastbeamnumber = 0;
	__int64 lastbeamnumber1, lastbeamnumber2, lastbeamnumber3;
	lastbeamnumber1 = lastbeamnumber2 = lastbeamnumber3 = 0;

	float scale, scale1, scale2, scale3; 
	unsigned int hits, hits1, hits2, hits3; 
	unsigned int gates, gates1, gates2, gates3; 

	unsigned int errors = 0; 
	unsigned int errors1, errors2, errors3; 
	errors1 = errors2 = errors3 = 0;

	// external timer card:
	TIMER ext_timer; // structure defining external timer parameters 
	unsigned int julian_day; 

// PMAC: 
HINSTANCE hLib_open;
DWORD PMACDevice;
//DWORD PMACDPRAMoffset; 
//#define COUNTS_PER_DEGREE 182.04444
//unsigned short PMACReceivedData;
//unsigned short PMACReceivedData1, PMACReceivedData2, PMACReceivedData3, PMACReceivedData4, PMACReceivedData5;
//float PMAC_az, PMAC_el; 
//unsigned short PMAC_scan_type, PMAC_sweep, PMAC_volume; 

// getsockopt parameters, initialization: 
  int iError;
  int level;        
  int optname;      
  int optlen;
  BOOL optvalBOOL; 
  int optvalint;
  UINT optvalUINT; 

//#define CARD_TESTING // test and use PCI card-detect functions
#ifdef CARD_TESTING // test and use PCI card-detect functions
U32 ReqLimit;
RETURN_CODE rc;
DEVICE_LOCATION Device;
// Query to get the total number of PLX devices
ReqLimit = FIND_AMOUNT_MATCHED;
// No search criteria, select all devices
Device.VendorId = PIRAQ_VENDOR_ID; // PLX Vendor ID
Device.DeviceId = PIRAQ_DEVICE_ID;
Device.VendorId = (U32)-1; // FIND Vendor ID
Device.DeviceId = (U32)-1;
Device.SerialNumber[0] = 0x00;
rc = PlxPciDeviceFind(&Device, &ReqLimit);
printf("ReqLimit = %d\n", ReqLimit);

// Query to get PLX device 0:
ReqLimit = 0;
// No search criteria, select all devices
Device.BusNumber = (U32)-1;
Device.SlotNumber = (U32)-1;
Device.VendorId = PIRAQ_VENDOR_ID; // PLX Vendor ID
Device.DeviceId = PIRAQ_DEVICE_ID;
Device.SerialNumber[0] = 0x00;
rc = PlxPciDeviceFind(&Device, &ReqLimit);
printf("Query to get PLX device 0: rc = 0x%x ReqLimit = %d Device.SlotNumber = %d Device.BusNumber = %d\n", rc, ReqLimit, Device.SlotNumber, Device.BusNumber);

// Query to get PLX device 1:
ReqLimit = 1;
// No search criteria, select all devices
Device.BusNumber = (U32)-1;
Device.SlotNumber = (U32)-1;
Device.VendorId = PIRAQ_VENDOR_ID; // PLX Vendor ID
Device.DeviceId = PIRAQ_DEVICE_ID;
Device.SerialNumber[0] = 0x00;
rc = PlxPciDeviceFind(&Device, &ReqLimit);
printf("Query to get PLX device 1: rc = 0x%x ReqLimit = %d Device.SlotNumber = %d Device.BusNumber = %d\n", rc, ReqLimit, Device.SlotNumber, Device.BusNumber);

// Query to get PLX device 2:
ReqLimit = 2;
// No search criteria, select all devices
Device.BusNumber = (U32)-1;
Device.SlotNumber = (U32)-1;
Device.VendorId = PIRAQ_VENDOR_ID; // PLX Vendor ID
Device.DeviceId = PIRAQ_DEVICE_ID;
Device.SerialNumber[0] = 0x00;
rc = PlxPciDeviceFind(&Device, &ReqLimit);
printf("Query to get PLX device 2: rc = 0x%x ReqLimit = %d Device.SlotNumber = %d Device.BusNumber = %d\n", rc, ReqLimit, Device.SlotNumber, Device.BusNumber);

//exit(0);  

ReqLimit = 3;
// No search criteria, select all devices
Device.BusNumber = (U32)-1;
Device.SlotNumber = (U32)-1;
Device.VendorId = (U32)-1; // PLX Vendor ID 
Device.DeviceId = (U32)-1;
Device.SerialNumber[0] = 0x00;
rc = PlxPciDeviceFind(&Device, &ReqLimit);
printf("Query to get PMAC: rc = 0x%x ReqLimit = %d Device.SlotNumber = %d Device.BusNumber = %d Device.VendorId = %x Device.DeviceId = %x\n", rc, ReqLimit, Device.SlotNumber, Device.BusNumber, Device.VendorId, Device.DeviceId);

ReqLimit = 4;
// No search criteria, select all devices
Device.BusNumber = (U32)-1;
Device.SlotNumber = (U32)-1;
Device.VendorId = (U32)-1; // PLX Vendor ID 
Device.DeviceId = (U32)-1;
Device.SerialNumber[0] = 0x00;
rc = PlxPciDeviceFind(&Device, &ReqLimit);
printf("Query to get PMAC: rc = 0x%x ReqLimit = %d Device.SlotNumber = %d Device.BusNumber = %d Device.VendorId = %x Device.DeviceId = %x\n", rc, ReqLimit, Device.SlotNumber, Device.BusNumber, Device.VendorId, Device.DeviceId);

ReqLimit = 5;
// No search criteria, select all devices
Device.BusNumber = (U32)-1;
Device.SlotNumber = (U32)-1;
Device.VendorId = (U32)-1; // PLX Vendor ID 
Device.DeviceId = (U32)-1;
Device.SerialNumber[0] = 0x00;
rc = PlxPciDeviceFind(&Device, &ReqLimit);
printf("Query to get PMAC: rc = 0x%x ReqLimit = %d Device.SlotNumber = %d Device.BusNumber = %d Device.VendorId = %x Device.DeviceId = %x\n", rc, ReqLimit, Device.SlotNumber, Device.BusNumber, Device.VendorId, Device.DeviceId);

exit(0); 
#endif

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}

// open debug data file: 
db_fp = fopen("debug.dat","w");
fprintf(db_fp,"go.exe results:\n");  

    printf("singlepiraq: usage -- \n"); 
    printf("             default output localhost\n"); 
    printf("             default outport 3100\n"); 
    printf("             parameters --\n"); 
    printf("             <DSP filename> <received data format: 'f' floats, 's' unsigned shorts>\n"); 
	if (argc > 1) { // entered a filename 
        if ((dspEXEC = fopen(argv[1],"r")) == NULL) // DSP executable file not found 
			{ printf("Usage: %s <DSP filename> else single_p3iq.out is used\n", argv[0]); exit(0); 
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
        else if (dspl_format == 'S') { // display short data 
			printf("SHORT format specified for piraq received data\n"); 
			printf("data displayed every 100 fifo hits\n"); 
			dspl_hits = 100; 
		} 
        else if (dspl_format =='A') { // display ASCII-graphic ABPDATA 
			printf("ASCII-graphic ABPDATA format specified for piraq received data\n"); dspl_hits = 100; 
			printf("dataformat = 16 required in config.dsp\n"); 

		} 
		else { // no data display
			printf("NO DISPLAY of piraq received data\n"); 
		} 
	}
	if (argc > 3) { // entered a piraq BOARD-enable mask (0-7)
		piraqs = atoi(argv[3]); 
		printf("piraq select mask = %d\n", piraqs); 
	}
	char fname[10]; // configuration filename
	if (argc > 4) { // entered a filename
		strcpy(fname, argv[4]);
// append .dsp and try: 
//		if (!fopen(fname, r)) { 
//			printf(" config filename %s does not exist\n", fname); 
//			exit(0); 
//		} 
	}
	else 
		strcpy(fname, "config"); 
	printf(" config filename %s will be used\n", fname); 

	// Open the PMAC using Windriver6 functions: 
    if (!pmac)
		PMAC_Open(&pmac,PMAC_DEFAULT_VENDOR_ID,PMAC_DEFAULT_DEVICE_ID,0);
	if (pmac) // PMAC Motion Controller found
		PMACDPRAM = (unsigned short *)PMAC_GetBasePtr(pmac, 0); 
	printf("PMAC_HANDLE pmac = 0x%x\n", pmac); 
	printf("Set PMACDPRAM = 0x%x\n", PMACDPRAM); 

#ifndef DC_OFFSET_TESTING  // remove transmitter control: temporary test if introduction produced errors
#ifndef TIMER_CARD_TESTING // turn the transmitter OFF here so first hit measures dc offset correctly
	printf("\n\nTURN TRANSMITTER OFF for piraq dc offset measurement.\nPress any key to continue.\n"); 
	while(!kbhit())	;
	c = toupper(getch()); // get the character
#endif
#endif

	// this if/else wraps around the entire main: else {...} contains single-board operation. 
	if (piraqs) { // entered slot pattern: bit set runs board, right-to-left 
		          // (right board most significant bit). 
		// use defaults: outport = 3100 outsock opens "localhost"
		outport = 3100; 
		iError = 0; WSASetLastError(iError);
iError = WSAGetLastError ();
printf("set/get iError = %d\n",iError);
	    if((outsock = open_udp_out("localhost")) ==  ERROR)			/* open one socket */
		    {printf("%s: Could not open output socket\n",name); exit(0);}
	    printf("%d: udp socket opens; outsock = %d\n", testnum, outsock); testnum++; 
iError = WSAGetLastError ();
printf("open_udp_out(): iError = %d\n",iError);

#if 0 // get/set socket options
level = SOL_SOCKET; 
optname = SO_BROADCAST; 
optlen = sizeof(BOOL); 
r_c = getsockopt(outsock, level, optname, (char *)&optvalBOOL, &optlen); 
printf("r_c = %d, outsock:%d, SOL_SOCKET, SO_BROADCAST: %d, optlen %d\n",r_c, outsock, optvalBOOL, optlen);
if (r_c) { 
	iError = WSAGetLastError ();
	printf("getsockopt(): iError = 0x%x\n",iError);
}

optname = SO_DONTLINGER;
optlen = sizeof(BOOL); 
r_c = getsockopt(outsock, level, optname, (char *)&optvalBOOL, &optlen); 
printf("r_c = %d, outsock:%d, SOL_SOCKET, SO_DONTLINGER: %d\n",r_c,outsock, optvalBOOL);
optname = SO_MAX_MSG_SIZE;
optlen = sizeof(UINT); 
r_c = getsockopt(outsock, level, optname, (char *)&optvalUINT, &optlen); 
printf("r_c = %d, outsock:%d, SOL_SOCKET, SO_MAX_MSG_SIZE: %d\n",r_c,outsock, optvalUINT);
optname = SO_SNDBUF;
optlen = sizeof(int); 
r_c = getsockopt(outsock, level, optname, (char *)&optvalint, &optlen); 
printf("r_c = %d, outsock:%d, SOL_SOCKET, SO_SNDBUF: %d\n",r_c,outsock, optvalint);
optname = SO_RCVBUF;
optlen = sizeof(int); 
r_c = getsockopt(outsock, level, optname, (char *)&optvalint, &optlen); 
printf("r_c = %d, outsock:%d, SOL_SOCKET, SO_RCVBUF: %d\n",r_c,outsock, optvalint);
optname = SO_TYPE;
optlen = sizeof(int); 
r_c = getsockopt(outsock, level, optname, (char *)&optvalint, &optlen); 
printf("r_c = %d, outsock:%d, SOL_SOCKET, SO_TYPE: %d\n",r_c,outsock, optvalint);
exit(0);
#endif
eof_start_over: 
#ifdef TIMER_CARD
		timer_stop(&ext_timer); // stop timer card 
#endif
		config1 = new CONFIG; config2 = new CONFIG; config3 = new CONFIG; 

	    piraq1 = new PIRAQ;

//		readconfig("config",config);   /* read in config.dsp, or use defaults if NULL passed. set up all parameters */ 
		readconfig(fname,config);   /* read in fname.dsp, or use config.dsp if NULL passed. set up all parameters */ 
// check gate limits hardcoded in piraq executable: 
		if (config->dataformat == 16) { // RapidDOW ABP
			if (config->gatesa > 400) {
				printf("gates = %d configured; max 400 for dataformat 16\n", config->gatesa); exit(0);
			} 
			bytespergate = 6 * sizeof(float); 
		}
		else if (config->dataformat == 17) { // Staggered PRT 
			if (config->gatesa > 600) {
				printf("gates = %d configured; max 600 for dataformat 17\n", config->gatesa); exit(0);
			} 
			bytespergate = 12 * sizeof(float); 
		}
		else if (config->dataformat == 18) { // CP2 Timeseries 
//			if (config->gatesa > 1000) {
//				printf("gates = %d configured; max 1000 for dataformat 18\n", config->gatesa); exit(0);
//			} 
			bytespergate = 2 * sizeof(float); 
			// CP2: compute #hits combined into one PCI Bus transfer
			Nhits = 65536 / (HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON); 
//			Nhits = 8; 
		}

		r_c = piraq1->Init(PIRAQ_VENDOR_ID,PIRAQ_DEVICE_ID); 
		printf("piraq1->Init() r_c = %d\n", r_c); 
		if (r_c == -1) { // how to use GetErrorString() 
			char errmsg[256]; 
			piraq1->GetErrorString(errmsg); printf("error: %s\n", errmsg); 
			piraqs &= ~0x0001; goto nop1; 
		} 
printf("Set PMACDPRAM = 0x%x\n", PMACDPRAM); 
		piraq1->SetPMACAntennaDPRAMAddress(PMACDPRAM); 
PMACDPRAM = piraq1->GetPMACAntennaDPRAMAddress(); 
printf("Get PMACDPRAM = 0x%x\n", PMACDPRAM); 
		/* put the DSP into a known state where HPI reads/writes will work */
		piraq1->ResetPiraq(); // !!!redundant? 
	    piraq1->GetControl()->UnSetBit_StatusRegister0(STAT0_SW_RESET);
		Sleep(1);
	    piraq1->GetControl()->SetBit_StatusRegister0(STAT0_SW_RESET);
	    Sleep(1);
		printf("piraq1 reset\n"); 
		unsigned int EPROM1[128]; 
		piraq1->ReadEPROM(EPROM1);
		for(y = 0; y < 16; y++) {
			printf("%08lX %08lX %08lX %08lX\n",EPROM1[y*4],EPROM1[y*4+1],EPROM1[y*4+2],EPROM1[y*4+3]); 
		}

		// if board 1 selected: 
		if (piraqs & 0x01) { // turn on slot 1
			stop_piraq(config, piraq1);
			fifo1 = (FIFO *)piraq1->GetBuffer(); 
//original: 			piraq_fifo_init(fifo1,"/PRQDATA", HEADERSIZE, HEADERSIZE+IQSIZE, PIRAQ_FIFO_NUM);
#ifndef	RUNTIME_PACKET_SIZING	// source project: data packets sized at build time.
			piraq_fifo_init(fifo1,"/PRQDATA", HEADERSIZE, HEADERSIZE+ABPSIZE, PIRAQ_FIFO_NUM); 
#else		// CP2: data packets sized at runtime.  + BUFFER_EPSILON
			piraq_fifo_init(fifo1,"/PRQDATA", HEADERSIZE, Nhits * (HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON), PIRAQ_FIFO_NUM); 
			printf("hit size = %d computed Nhits = %d\n", (HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON), 65536 / (HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON)); 
#endif
			if (!fifo1) { printf("piraq1 fifo_create failed\n"); exit(0);
			}
			printf("fifo1 = %p, recordsize = %d\n", fifo1, fifo1->record_size); 
			pkt1 = (PACKET *)fifo_get_header_address(fifo1); 
			pkt1->cmd.flag = 0; // Preset the flags just in case
			pn_pkt = pkt1; // set live packet pointer for subsequent UNIX-epoch pulsenumber calculation
			pkt1->data.info.channel = 0;			// set BOARD number
			struct_init(&pkt1->data.info, fname);   /* initialize the info structure */
//printf("i_offset = %f\n\n", pkt1->data.info.spare[0]); exit(0); 
			r_c = piraq1->LoadDspCode(argv[1]); // load entered DSP executable filename
			printf("loading %s: piraq1->LoadDspCode returns %d\n", argv[1], r_c);  
			timerset(config, piraq1); // !note: also programs pll and FIR filter. 
			printf("Opening FIFO /CMD1......"); 
			cmd1 = fifo_create("/CMD1",0,HEADERSIZE,CMD_FIFO_NUM);
			if(!cmd1)	{printf("\nCannot open /CMD1 FIFO buffer\n"); exit(-1);}
			cmd1->port = CMD_RING_PORT+1;
			printf("   done.\n");
			/* make sure command socket is last file descriptor opened */
			if((cmd1_notifysock = open_udp_in(cmd1->port)) == ERROR) { /* open the input socket on port where data will come from */
				printf("%s: Could not open piraq1 notification socket\n",argv[0]); exit(-1);
			}
			printf("cmd1_notifysock = %d \n", cmd1_notifysock); 
		} // end if board 1 selected: 
#ifdef TIMER_CARD_TESTING
		start_timer_card(&ext_timer, &pn_pkt->data.info); 
		printf("\n\nprt  = %8.3e\n", pn_pkt->data.info.prt[0]); // pn_pkt = PACKET pointer to a live piraq 
		printf("prt2 = %8.3e\n", pn_pkt->data.info.prt[1]); 
		exit(0); 
#endif
nop1:
		piraq2 = new PIRAQ;

		r_c = piraq2->Init(PIRAQ_VENDOR_ID,PIRAQ_DEVICE_ID);
		printf("piraq2->Init() r_c = %d\n", r_c);
		if (r_c == -1) { 
			char errmsg[256]; 
			piraq2->GetErrorString(errmsg); printf("error: %s\n", errmsg); 
			piraqs &= ~0x0002; goto nop2; 
		} 
printf("Set PMACDPRAM = 0x%x\n", PMACDPRAM); 
		piraq2->SetPMACAntennaDPRAMAddress(PMACDPRAM); 
PMACDPRAM = piraq2->GetPMACAntennaDPRAMAddress(); 
printf("Get PMACDPRAM = 0x%x\n", PMACDPRAM); 
		piraq2->ResetPiraq(); 
	    piraq2->GetControl()->UnSetBit_StatusRegister0(STAT0_SW_RESET);
		Sleep(1);
	    piraq2->GetControl()->SetBit_StatusRegister0(STAT0_SW_RESET);
	    Sleep(1);
		printf("piraq2 reset\n"); 
		unsigned int EPROM2[128]; 
		piraq2->ReadEPROM(EPROM2);
		for(y = 0; y < 16; y++) {
			printf("%08lX %08lX %08lX %08lX\n",EPROM2[y*4],EPROM2[y*4+1],EPROM2[y*4+2],EPROM2[y*4+3]); 
		}
		// if board 2 selected: 
		if (piraqs & 0x02) { // turn on slot 2
		    stop_piraq(config, piraq2);
		    fifo2 = (FIFO *)piraq2->GetBuffer(); 
//original:			piraq_fifo_init(fifo2,"/PRQDATA", HEADERSIZE, HEADERSIZE+IQSIZE, PIRAQ_FIFO_NUM); // replaces fifo_create()
#ifndef	RUNTIME_PACKET_SIZING	// source project: data packets sized at build time.
			piraq_fifo_init(fifo2,"/PRQDATA", HEADERSIZE, HEADERSIZE+ABPSIZE, PIRAQ_FIFO_NUM); // replaces fifo_create()
#else		// CP2: data packets sized at runtime. 
			piraq_fifo_init(fifo2,"/PRQDATA", HEADERSIZE, Nhits * (HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON), PIRAQ_FIFO_NUM); 
#endif
			if (!fifo2)	{
				printf("piraq2 fifo_create failed\n");      exit(0);
			}
			printf("fifo2 = %p, recordsize = %d\n", fifo2, HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON); 
			pkt2 = (PACKET *)fifo_get_header_address(fifo2);
			pkt2->cmd.flag = 0; // Preset the flags just in case
			pn_pkt = pkt2; // set live packet pointer for subsequent UNIX-epoch pulsenumber calculation
			pkt2->data.info.channel = 1;			// set BOARD number
			struct_init(&pkt2->data.info, fname);   /* initialize the info structure */
			r_c = piraq2->LoadDspCode(argv[1]); // load entered DSP executable filename
			printf("loading %s: piraq2->LoadDspCode returns %d\n", argv[1], r_c);  
			timerset(config, piraq2); // !note: also programs pll and FIR filter. 
			printf("Opening FIFO /CMD2......"); 
			cmd2 = fifo_create("/CMD2",0,HEADERSIZE,CMD_FIFO_NUM);
			if(!cmd2) {
				printf("\nCannot open /CMD2 FIFO buffer\n"); exit(-1);
			}
			cmd2->port = CMD_RING_PORT+2;
			printf("   done.\n");
		   /* make sure command socket is last file descriptor opened */
			if((cmd2_notifysock = open_udp_in(cmd2->port)) == ERROR) {	/* open the input socket on port where data will come from */
				printf("%s: Could not open piraq2 notification socket\n",argv[0]); exit(-1);
			}
			printf("cmd2_notifysock = %d \n", cmd2_notifysock); 
		} // end if board 2 selected: 
nop2: 
#ifdef TIMER_PIRAQ // worked here
//#ifdef TIMER_PIRAQ_third_from_left
	    timer_piraq = new PIRAQ;

		r_c = timer_piraq->Init(PIRAQ_VENDOR_ID,PIRAQ_DEVICE_ID);
		printf("timer_piraq->Init() r_c = %d\n", r_c); 
		if (r_c == -1) { // how to use GetErrorString() 
			char errmsg[256]; 
			timer_piraq->GetErrorString(errmsg); printf("error: %s\n", errmsg); exit(0);
		} 
		/* put the DSP into a known state */
		timer_piraq->ResetPiraq(); // !!!redundant? 
	    timer_piraq->GetControl()->UnSetBit_StatusRegister0(STAT0_SW_RESET);
		Sleep(1);
	    timer_piraq->GetControl()->SetBit_StatusRegister0(STAT0_SW_RESET);
	    Sleep(1);
		printf("timer_piraq reset\n"); 
		stop_piraq(config, timer_piraq);
		timerset(config, timer_piraq); // !note: also programs pll and FIR filter. 
#endif		

		piraq3 = new PIRAQ;

		r_c = piraq3->Init(PIRAQ_VENDOR_ID,PIRAQ_DEVICE_ID);
		printf("piraq3->Init() r_c = %d\n", r_c); 
		if (r_c == -1) { // how to use GetErrorString() 
			char errmsg[256]; 
			piraq3->GetErrorString(errmsg); printf("error: %s\n", errmsg); 
			piraqs &= ~0x0004; goto nop3; 
		} 
printf("Set PMACDPRAM = 0x%x\n", PMACDPRAM); 
		piraq3->SetPMACAntennaDPRAMAddress(PMACDPRAM); 
PMACDPRAM = piraq3->GetPMACAntennaDPRAMAddress(); 
printf("Get PMACDPRAM = 0x%x\n", PMACDPRAM); 
		/* put the DSP into a known state where HPI reads/writes will work */
		piraq3->ResetPiraq(); // !!!redundant? 
	    piraq3->GetControl()->UnSetBit_StatusRegister0(STAT0_SW_RESET);
		Sleep(1);
	    piraq3->GetControl()->SetBit_StatusRegister0(STAT0_SW_RESET);
	    Sleep(1);
		printf("piraq3 reset\n"); 
		unsigned int EPROM3[128]; 
		piraq3->ReadEPROM(EPROM3);
		for(y = 0; y < 16; y++) {
			printf("%08lX %08lX %08lX %08lX\n",EPROM3[y*4],EPROM3[y*4+1],EPROM3[y*4+2],EPROM3[y*4+3]); 
		}
		// if board 3 selected: 
		if (piraqs & 0x04) { // turn on slot 3
		    stop_piraq(config, piraq3);
		    fifo3 = (FIFO *)piraq3->GetBuffer(); 
//original:			piraq_fifo_init(fifo3,"/PRQDATA", HEADERSIZE, HEADERSIZE+IQSIZE, PIRAQ_FIFO_NUM); // replaces fifo_create()
#ifndef	RUNTIME_PACKET_SIZING	// source project: data packets sized at build time.
			piraq_fifo_init(fifo3,"/PRQDATA", HEADERSIZE, HEADERSIZE+ABPSIZE, PIRAQ_FIFO_NUM); // replaces fifo_create()
#else		// CP2: data packets sized at runtime. 
			piraq_fifo_init(fifo3,"/PRQDATA", HEADERSIZE, Nhits * (HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON), PIRAQ_FIFO_NUM); 
#endif
			if (!fifo3) {
				printf("piraq3 fifo_create failed\n");      exit(0);
			}
			printf("fifo3 = %p, recordsize = %d\n", fifo3, HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON); 
			pkt3 = (PACKET *)fifo_get_header_address(fifo3);
			pkt3->cmd.flag = 0; // Preset the flags just in case
			pn_pkt = pkt3; // set live packet pointer for subsequent UNIX-epoch pulsenumber calculation
			pkt3->data.info.channel = 2;			// set BOARD number
			struct_init(&pkt3->data.info, fname);   /* initialize the info structure */
			r_c = piraq3->LoadDspCode(argv[1]); // load entered DSP executable filename
			printf("loading %s: piraq3->LoadDspCode returns %d\n", argv[1], r_c);  
			timerset(config, piraq3); // !note: also programs pll and FIR filter. 
			printf("Opening FIFO /CMD3......"); 
			cmd3 = fifo_create("/CMD3",0,HEADERSIZE,CMD_FIFO_NUM);
			if(!cmd3)	{printf("\nCannot open /CMD3 FIFO buffer\n"); exit(-1);}
			cmd3->port = CMD_RING_PORT+3;
			printf("   done.\n");
		   /* make sure command socket is last file descriptor opened */
			if((cmd3_notifysock = open_udp_in(cmd3->port)) == ERROR)	/* open the input socket on port where data will come from */
				{printf("%s: Could not open piraq3 notification socket\n",argv[0]); exit(-1);
			}
			printf("cmd3_notifysock = %d \n", cmd3_notifysock); 
		} // end if board 3 selected
nop3:
#ifdef TIMER_PIRAQ_fourth_from_left
//#ifdef TIMER_PIRAQ
	    timer_piraq = new PIRAQ;

		r_c = timer_piraq->Init(PIRAQ_VENDOR_ID,PIRAQ_DEVICE_ID);
		printf("timer_piraq->Init() r_c = %d\n", r_c); 
		if (r_c == -1) { // how to use GetErrorString() 
			char errmsg[256]; 
			timer_piraq->GetErrorString(errmsg); printf("error: %s\n", errmsg); exit(0);
		} 
		/* put the DSP into a known state */
		timer_piraq->ResetPiraq(); // !!!redundant? 
	    timer_piraq->GetControl()->UnSetBit_StatusRegister0(STAT0_SW_RESET);
		Sleep(1);
	    timer_piraq->GetControl()->SetBit_StatusRegister0(STAT0_SW_RESET);
	    Sleep(1);
		printf("timer_piraq reset\n"); 
		stop_piraq(config, timer_piraq);
		timerset(config, timer_piraq); // !note: also programs pll and FIR filter. 
#endif		
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
		hits = config->hits; 
		fpsuppm = fpprf/(double)hits; 
		fpExactmSecperBeam = ((double)1000.0)/fpsuppm; 
		printf("prt = %+8.3e fpprf = %+8.3e fpsuppm = fpprf/hits = %+8.3e\n", prt, fpprf, fpsuppm); 
		printf("fpExactmSecperBeam = %8.3e\n", fpExactmSecperBeam); 

		pri = (unsigned int)(((float)COUNTFREQ)/(float)(1/prt)) + 0.5; 
		printf("pri = %d\n", pri); 
		printf("prf = %I64d\n", prf); 
		printf("prt2 = %8.3e\n", pn_pkt->data.info.prt[1]); 
		hits = config->hits; 
#if 1
		printf("hits = %d\n", hits); 
		float suppm; suppm = prf/(float)hits; 
		printf("ceil(prf/hits) = %4.5f, prf/hits = %4.5f\n", ceil(prf/hits), suppm); 
//		if (ceil(prf/hits) != suppm) {
//			printf("integral beams/sec required\n"); exit(0); 
//		} 
#endif
#ifdef NO_INTEGER_BEAMS
goto no_int_beams; 
#endif
		BeamsperSec = (unsigned int)prf/hits; 
		mSecperBeam = 1000/BeamsperSec; 
		ExactmSecperBeam = 1000.0/suppm; 
		printf("ExactmSecperBeam = %4.3f\n", ExactmSecperBeam); 
//	use integer implementation until full f.p. method is designed: 2-7-05 mp. 
//		printf("current implementation requires integer #beams/sec\n"); 
//		if (ceil(ExactmSecperBeam) != mSecperBeam) {
//			printf("integral milliseconds per beam required\n"); exit(0); 
//		} 
		printf("BeamsperSec = %d mSecperBeam = %d\n", BeamsperSec, mSecperBeam); 
no_int_beams:
		// get current second and wait for it to pass; 
		now = time(&now); now_was = now;
		while(now == now_was) // current second persists 
			now = time(&now);	//  
		now = time(&now); now_was = now;
		_ftime( &timebuffer );
		SystemEpochMillisec = (timebuffer.time * (__int64)1000) + (__int64)timebuffer.millitm;
		printf("before piraq start():\nSystemEpochMillisec = %I64d\n", SystemEpochMillisec); 
		printf("now WILL BE %I64d\n", timebuffer.time + (__int64)2); 
		pulsenum = ((((__int64)(now+2)) * (__int64)COUNTFREQ) / pri) + 1; 
		beamnum = pulsenum / hits;
//beamnum += (__int64)(TimerStartCorrection/mSecperBeam); // cannot do this here ... do at interpolation time
		printf("pulsenum=%I64d\n", pulsenum); 
		printf("beamnum=%I64d\n", beamnum); 
		// start the piraqs, waiting for each to indicate functionality: 
		if (piraqs & 0x01) { // turn on slot 1
#if 1	// 0: restore previous beamnum method
			pkt1->data.info.pulse_num = pulsenum;	// set UNIX epoch pulsenum just before starting
			pkt1->data.info.beam_num = beamnum; 
			pkt1->data.info.packetflag = 1;			// set to piraq: get header! 
printf("board%d: receiver_gain = %4.2f vreceiver_gain = %4.2f \n", pkt1->data.info.channel, pkt1->data.info.receiver_gain, pkt1->data.info.vreceiver_gain); 
printf("board%d: noise_power = %4.2f vnoise_power = %4.2f \n", pkt1->data.info.channel, pkt1->data.info.noise_power, pkt1->data.info.vnoise_power); 
#endif
			if (!start(config,piraq1,pkt1)) 		  /* start the PIRAQ: also points the piraq to the fifo structure */ 
				{printf("\npiraq1 DSP program not ready: pkt1->cmd.flag != TRUE (1)\n"); exit(-1);}
		} 
		if (piraqs & 0x02) { // turn on slot 2
#if 1	// 0: restore previous beamnum method
			pkt2->data.info.pulse_num = pulsenum;	// set UNIX epoch pulsenum just before starting
			pkt2->data.info.beam_num = beamnum; 
			pkt2->data.info.packetflag = 1;			// set to piraq: get header! 
printf("board%d: receiver_gain = %4.2f vreceiver_gain = %4.2f \n", pkt2->data.info.channel, pkt2->data.info.receiver_gain, pkt2->data.info.vreceiver_gain); 
printf("board%d: noise_power = %4.2f vnoise_power = %4.2f \n", pkt2->data.info.channel, pkt2->data.info.noise_power, pkt2->data.info.vnoise_power); 
#endif
			if (!start(config,piraq2,pkt2)) 		  /* start the PIRAQ: also points the piraq to the fifo structure */ 
				{printf("\npiraq2 DSP program not ready: pkt2->cmd.flag != TRUE (1)\n"); exit(-1);}
		} 
		if (piraqs & 0x04) { // turn on slot 3
#if 1	// 0: restore previous beamnum method
			pkt3->data.info.pulse_num = pulsenum;	// set UNIX epoch pulsenum just before starting
			pkt3->data.info.beam_num = beamnum; 
			pkt3->data.info.packetflag = 1;			// set to piraq: get header! 
printf("board%d: receiver_gain = %4.2f vreceiver_gain = %4.2f \n", pkt3->data.info.channel, pkt3->data.info.receiver_gain, pkt3->data.info.vreceiver_gain); 
printf("board%d: noise_power = %4.2f vnoise_power = %4.2f \n", pkt3->data.info.channel, pkt3->data.info.noise_power, pkt3->data.info.vnoise_power); 
#endif
			if (!start(config,piraq3,pkt3)) 		  /* start the PIRAQ: also points the piraq to the fifo structure */ 
				{printf("\npiraq3 DSP program not ready: pkt3->cmd.flag != TRUE (1)\n"); exit(-1);}
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
		// set pulsenum, beamnum computed from PPS-edge second in all boards requested and started;  
#if 0	// 1: restore previous method
		if (piraqs & 0x01) { // piraq1 selected 
			pkt1->data.info.pulse_num = pulsenum;	// set UNIX epoch pulsenum just before starting
			pkt1->data.info.beam_num = beamnum; 
			pkt1->data.info.packetflag = 1;			// set to piraq: get header! 
			pkt1->data.info.channel = 0;			// set BOARD number
		} 
		if (piraqs & 0x02) { // piraq2 selected 
			pkt2->data.info.pulse_num = pulsenum; // set UNIX epoch pulsenum just before starting
			pkt2->data.info.beam_num = beamnum; 
			pkt2->data.info.packetflag = 1;			// set to piraq: get header! 
			pkt2->data.info.channel = 1;			// set BOARD number
		} 
		if (piraqs & 0x04) { // piraq3 selected 
			pkt3->data.info.pulse_num = pulsenum; // set UNIX epoch pulsenum just before starting
			pkt3->data.info.beam_num = beamnum; 
			pkt3->data.info.packetflag = 1;			// set to piraq: get header! 
			pkt3->data.info.channel = 2;			// set BOARD number
		}
#endif
		// time-related parameters initialized -- wait for new second then start timer card and 
		// data acquisition. 
		printf("now=%d: ... still waiting for this second (now) to expire ... then timer will start\n\n", now);  
		while(now == now_was) // current second persists 
			now = time(&now); 
		printf("now=%d: ... now_was=%d\n", now, now_was);  
		// start timer board immediately:
// !note: operator must synchronize system clock w/Epsilon so PPS edge happens on second change. 
// !clarify: ? start timer card BEFORE ? does PPS edge require start_timer_card already executed?
					_ftime( &timebuffer );
					timeline = ctime( & ( timebuffer.time ) );
					printf( "mSec=%hu\n", timebuffer.millitm );
#ifdef TIMER_CARD
		start_timer_card(&ext_timer, &pn_pkt->data.info); // pn_pkt = PACKET pointer to a live piraq 
#endif

		_ftime( &timebuffer );
		timeline = ctime( & ( timebuffer.time ) );
		printf( "timer start correction mSec=%hu\n", timebuffer.millitm );
		TimerStartCorrection = timebuffer.millitm;
		
_ftime( &timebuffer );
SystemEpochMillisec = (timebuffer.time * (__int64)1000) + (__int64)timebuffer.millitm;
RadarEpochMillisec = mSecperBeam * pn_pkt->data.info.beam_num; // pn_pkt: access a live packet
//!!!LATER:RadarEpochMillisec = (__int64)(ExactmSecperBeam * ((float)(pn_pkt->data.info.beam_num))); // pn_pkt: access a live packet; (double)?

_ftime( &timebuffer );
printf("1:\nRadarEpochMillisec  = %I64d \nSystemEpochMillisec = %I64d\n", RadarEpochMillisec, SystemEpochMillisec); 

fpSystemMillisec = ((double)timebuffer.time * 1000.0) + (double)timebuffer.millitm;
fpRadarMillisec = fpExactmSecperBeam * (double)pn_pkt->data.info.beam_num; // pn_pkt: access a live packet
printf("1:\nfpRadarMillisec          = %+8.12e\n", fpRadarMillisec); 
printf("1:\nfpSystemMillisec         = %+8.12e\n", fpSystemMillisec); 
fpRadarSystemCorrection = (fpSystemMillisec - (double)TimerStartCorrection) - fpRadarMillisec; 
printf("1:\nfpRadarSystemCorrection  = %+8.5e\n", fpRadarSystemCorrection); 
printf("1:\nfpRadarMillisecCorrected = %+8.12e\n", fpRadarMillisec + fpRadarSystemCorrection); 
RadarEpochMillisec = (__int64)(fpRadarMillisec + fpRadarSystemCorrection); 

#ifndef DC_OFFSET_TESTING // remove transmitter control: temporary test if introduction produced errors
//	printf("\n\nTURN TRANSMITTER ON. Data acquisition started. \nPress any key to continue.\n"); 
//	while(!kbhit())	;
//	c = toupper(getch()); // get the character
#endif

#ifdef TIMER_PIRAQ 
        timer_piraq->GetControl()->UnSetBit_StatusRegister0((STAT0_TRESET) | (STAT0_TMODE));
	    timer_piraq->GetControl()->UnSetBit_StatusRegister0(STAT0_TMODE);
        #define STOPDELAY 30
        Sleep(STOPDELAY);    /* wait some number of milliseconds */
        timer_piraq->GetControl()->SetBit_StatusRegister0(STAT0_TRESET);

        timer_piraq->GetControl()->SetBit_StatusRegister0(STAT0_TMODE);
		Sleep(1);
        timer_piraq->GetControl()->UnSetBit_StatusRegister0(STAT0_TMODE);
#endif
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
if (((cur_fifo1_hits % 2) == 0) || (cur_fifo1_hits < 2)) { printf("b0: hits1 = %d\n", cur_fifo1_hits); } // print often enough ... on atd-milan
					cycle_fifo1_hits++; 
					fifopiraq1 = (PACKET *)fifo_get_read_address(fifo1,0); 
					if (fakeangles == FALSE) {
						r_c = pmac_WD6_acquire(fifopiraq1, TRUE); 
						if (r_c == FALSE) { printf("pmac_WD6_acquire() r_c = FALSE\n"); break; } // interpolation second linear point not yet available 
					}

#ifdef EOF_DETECT
					if ((int)fifopiraq1->data.info.packetflag == -1) { // piraq detected a hardware out-of-sync condition "EOF" 
						printf("\n\npiraq1 EOF detected. exiting.\n"); 
						if (piraqs & 0x01) { // slot 1 active
							stop_piraq(config, piraq1); 
						} 
						printf("\n\npiraq1 stopped\n"); 
						if (piraqs & 0x02) { // slot 2 active
							stop_piraq(config, piraq2); 
						} 
						if (piraqs & 0x04) { // slot 3 active
							stop_piraq(config, piraq3); 
						} 
						printf("\n\npiraqs stopped\n"); 
#ifdef TIMER_CARD
						timer_stop(&ext_timer); 
#endif
						delete piraq1; delete piraq2; delete piraq3; 
						printf("\n\ngoing to eof_start_over\n"); 
						goto eof_start_over; 
					}
#endif // end #ifdef EOF_DETECT
					hits1 = fifopiraq1->data.info.hits; gates1 = fifopiraq1->data.info.gates; 
					scale1 = (float)(PIRAQ3D_SCALE*PIRAQ3D_SCALE*hits1); // scale fifo1 data 
					udp1 = &fifopiraq1->udp;
					udp1->magic = MAGIC;
					if (config->dataformat == PIRAQ_ABPDATA) { // PIRAQ_ABPDATA: RAPID DOW
						udp1->type = UDPTYPE_PIRAQ_ABPDATA; 
						fifopiraq1->data.info.bytespergate = 6 * (sizeof(float)); // GRG ABPDATA
						fsrc = (float *)fifopiraq1->data.data; 
#ifdef	DRX_PACKET_TESTING	// define to activate data packet resizing for throughput testing. 
//						for (i = 0; i < 6*gates1 + 6*fifopiraq1->data.info.clutter_end[0]*fifopiraq1->data.info.gates; i++) {	// vary multiplier; note CPU usage in Task Manager. 6 = data/gate
//							*fsrc *= scale1; fsrc++;		// scale the data using multiplication
//						} 
#else
						for (i = 0; i < 6*gates1; i++) {	// 6 data/gate
							*fsrc /= scale1; fsrc++;		// scale the data
						} 
#endif
						if ((int)fifopiraq1->data.info.ts_start_gate != -1) { // diagnostic timeseries appended 
							int tsgates1 = (fifopiraq1->data.info.ts_end_gate - fifopiraq1->data.info.ts_start_gate) + 1; 
#ifndef TESTING_TIMESERIES // ordinary diagnostic timeseries data scaling: 
							float max_i = 0.0, max_q = 0.0; 
							for (i = 0; i < 4*hits1*tsgates1; i++) {	// 4 data/tsgate
//if (!(i%4)) fprintf(db_fp,"\n");
								*fsrc /= PIRAQ3D_SCALE; 	// scale the data -- 1 hit
//fprintf(db_fp,"i=%d *fsrc=%4.3f ",i,*fsrc); 
								fsrc++;	
							} 
//fprintf(db_fp,"\n\n"); 
//fprintf(db_fp,"max_i = %+8.3e max_q = %+8.3e\n",max_i,max_q); 
#endif
#ifdef TESTING_TIMESERIES // compute test diagnostic timeseries data in one of two piraq channels: 
						// compute pointer to diagnostic timeseries (I,Q) data  !note: had +4 between last two parens ... hmmmm....
							float max_i = 0.0, max_q = 0.0; 
//							int j = 0; // step for test data
#if 1 // sinusoidal test timeseries: 
							for (i = 0; i < hits1*tsgates1; i++) { // generate complete set of test diagnostic timeseries: !ONE CHANNEL ONLY 
								// scale CH1 data from piraq, put test timeseries in CH2: 
//								*fsrc /= PIRAQ3D_SCALE; fsrc++;	*fsrc /= PIRAQ3D_SCALE; fsrc++;
								// generate single frequency in all timeseries data: 
//								*fsrc = test_ts_power*(cos((float(TWOPI*idx_tts))/(10.0 + test_ts_adjust))); // !note: j=multiple of 100 gives exact max or min 
								// ADD single frequency in all timeseries data: 
								*fsrc /= PIRAQ3D_SCALE; *fsrc += test_ts_power*(cos((float(TWOPI*idx_tts))/(10.0 + test_ts_adjust))); // !note: j=multiple of 100 gives exact max or min 
								// generate unique frequency for each GATE: 
//								*fsrc = test_ts_power*(cos((float(TWOPI*idx_tts))/(10.0+test_ts_adjust+(i%tsgates1)))); 
								// ADD unique frequency for each GATE: 
//								*fsrc /= PIRAQ3D_SCALE; *fsrc += test_ts_power*(cos((float(TWOPI*idx_tts))/(10.0+test_ts_adjust+(i%tsgates1)))); 
								fsrc++;	
//								*fsrc = test_ts_power*(sin((float(TWOPI*idx_tts))/(10.0 + test_ts_adjust)));  
								*fsrc /= PIRAQ3D_SCALE; *fsrc += test_ts_power*(sin((float(TWOPI*idx_tts))/(10.0 + test_ts_adjust)));  
//								*fsrc = test_ts_power*(sin((float(TWOPI*idx_tts))/(10.0+test_ts_adjust+(i%tsgates1)))); 
//								*fsrc /= PIRAQ3D_SCALE; *fsrc += test_ts_power*(sin((float(TWOPI*idx_tts))/(10.0+test_ts_adjust+(i%tsgates1)))); 
								fsrc++;
								// scale CH2 data from piraq, put test timeseries in CH1: 
								*fsrc /= PIRAQ3D_SCALE; fsrc++;	*fsrc /= PIRAQ3D_SCALE; fsrc++;	
								idx_tts++; 
							}
	printf("test_ts_adjust = %d\n", test_ts_adjust); 
#else // simple ramp, over all timeseries, 0.0 - 1.0: 
							for (i = 0; i < 4*hits1*tsgates1; i++) { // generate complete set of test diagnostic timeseries: !ONE CHANNEL ONLY 
								// scale CH1 data from piraq, put test timeseries in CH2: 
								*fsrc = (5.0*(float)i)/((float)4*hits1*tsgates1); fsrc++;
								j++; 
							}
#endif
//fprintf(db_fp,"end i = %d max_i = %+8.3e max_q = %+8.3e\n",i,max_i,max_q); // save max I,Q  
//fprintf(db_fp,"end i=%d *fsrc-10=%+8.3e *fsrc-5=%+8.3e -4=%+8.3e -3=%+8.3e -2=%+8.3e -1=%+8.3e -0=%+8.3e\n",i,*(fsrc-10),*(fsrc-5),*(fsrc-4),*(fsrc-3),*(fsrc-2),*(fsrc-1),*(fsrc-0)); // save max I,Q  
#endif
						}
					}
					else if (config->dataformat == PIRAQ_ABPDATA_STAGGER_PRT) { // PIRAQ_ABPDATA w/Staggered PRT 
						udp1->type = UDPTYPE_PIRAQ_ABPDATA_STAGGER_PRT; 
						fifopiraq1->data.info.bytespergate = 12 * (sizeof(float)); // Staggered PRT ABPDATA
						fsrc = (float *)fifopiraq1->data.data; 
						for (i = 0; i < 12*gates1; i++) {	// 6 data/gate, twice
							*fsrc /= scale1; fsrc++;		// scale the data
						}
						if ((int)fifopiraq1->data.info.ts_start_gate != -1) { // diagnostic timeseries appended 
							int tsgates1 = (fifopiraq1->data.info.ts_end_gate - fifopiraq1->data.info.ts_start_gate) + 1; 
							for (i = 0; i < 4*hits1*tsgates1; i++) {	// 4 data/tsgate
								*fsrc /= PIRAQ3D_SCALE; fsrc++;		// scale the data -- 1 hit
							} 
						}
					}
					else if (config->dataformat == PIRAQ_CP2_TIMESERIES) { // CP2 Timeseries 
						udp1->type = UDPTYPE_PIRAQ_CP2_TIMESERIES; 
						fifopiraq1->data.info.bytespergate = 2 * (sizeof(float)); // Staggered PRT ABPDATA
						fsrc = (float *)fifopiraq1->data.data; 
						// no data scaling in CP2.exe
					}
					else {                 // short-integer IQDATA: time series
						udp1->type = UDPTYPE_IQDATA; 
						fifopiraq1->data.info.bytespergate = 4 * (sizeof(short)); // IQDATA 
					}

					temp1 = fifopiraq1->data.info.pulse_num * (unsigned __int64)(prt * (float)COUNTFREQ + 0.5);
					fifopiraq1->data.info.secs = temp1 / COUNTFREQ;
					fifopiraq1->data.info.nanosecs = ((unsigned __int64)10000 * (temp1 % ((unsigned __int64)COUNTFREQ))) / (unsigned __int64)COUNTFREQ;
					fifopiraq1->data.info.nanosecs *= (unsigned __int64)100000; // multiply by 10**5 to get nanoseconds
	                if (fakeangles == TRUE) { // no PMAC: fakin it from here ...  
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
					} 
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

#if 1	// test N-hit packet: 0: reduce printing
				__int64 * __int64_ptr, * __int64_ptr2; unsigned int * uint_ptr; float * fsrc2; 
				for (i = 0; i < Nhits; i++) { // all hits in the packet 
					// compute pointer to datum in an individual hit, dereference and print. 
					// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON)
					__int64_ptr = (__int64 *)((char *)&fifopiraq1->data.info.beam_num + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					beamnum = *__int64_ptr; 
					__int64_ptr2 = (__int64 *)((char *)&fifopiraq1->data.info.pulse_num + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					pulsenum = *__int64_ptr2; 
					uint_ptr = (unsigned int *)((char *)&fifopiraq1->data.info.hits + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					j = *uint_ptr; 
					uint_ptr = (unsigned int *)((char *)&fifopiraq1->data.info.channel + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					k = *uint_ptr; 
					fsrc2 = (float *)((char *)fifopiraq1->data.data + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON)));
					fsrc2 += 0; // look at THIS datum
					if (lastpulsenumber1 != pulsenum - 1) { // PNs not sequential
						printf("hit%d: lastPN = %I64d PN = %I64d\n", i+1, lastpulsenumber1, pulsenum); 
						fprintf(db_fp, "%d:hit%d: lastPN = %I64d PN = %I64d\n", fifopiraq1->data.info.channel, i+1, lastpulsenumber1, pulsenum); 
					} 
				// test beamnumbers sequential
//				if (beamnum != pulsenum / j) { // BN != PN/hits
//					printf("hit%d: computed BN1 = %I64d BN1 = %I64d PN1 = %I64d\n", i+1, pulsenum / j, beamnum, pulsenum); 
//				} 
				// print data once per fifo hit -- per N-hit packet: 
//				if (i == 0) { // 
//					printf("hit%d: BN = %I64d PN = %I64d hits = %d gates = %d fsrc2[0] = %+8.3f\n", i+1, beamnum, pulsenum, j, k, *fsrc2); 
//				} 
					lastpulsenumber1 = pulsenum; // previous hit PN
//					printf("hit%d: BN = %I64d PN = %I64d hits = %d gates = %d fsrc2[6] = %+8.3f\n", i+1, beamnum, pulsenum, j, k, *fsrc2); 
//					printf("%d: hit%02d: BN = %I64d PN = %I64d fsrc2[0] = %+8.3f fsrc2[1] = %+8.3f fsrc2[2] = %+8.3f fsrc2[end] = %+8.3f\n", k, i+1, beamnum, pulsenum, *fsrc2, fsrc2[1], fsrc2[2], fsrc2[(2*fifopiraq1->data.info.gates)-1]); 
				}
//				printf("\n"); 
#endif
						if ((testnum % dspl_hits) == 0) { // done dspl_hit cycles
						if(dspl_format != 'A') {
//							printf("%d: az1 = %4.1f el1 = %4.1f scan1 = %d volume1 = %d\n", testnum, az1, el1, scan1, volume1); 
//						    printf("board = %d:\n", fifopiraq1->data.info.channel);
//						    printf("rcvr_pulsewidth = %+8.3e xmit_pulsewidth = %+8.3e\n", 
//									fifopiraq1->data.info.rcvr_pulsewidth,
//									fifopiraq1->data.info.xmit_pulsewidth);
						} // end	if(dspl_format != 'A')
					    if (dspl_format == 'S') { // display short data 
							ssrc = (unsigned short *)fifopiraq1->data.data; 
							printf("ssrc0 = 0x%x\n",&ssrc[0]); 
					        printf("ssrc0 = 0x%x ssrc1 = 0x%x ssrc2 = 0x%x ssrc3 = 0x%x ssrc4 = 0x%x ssrc5 = 0x%x ssrc6 = 0x%x ssrc7 = 0x%x\n",
				  			  ssrc[0], ssrc[1], ssrc[2], ssrc[3], ssrc[4], ssrc[5], ssrc[6], ssrc[7]); 
//						    printf("ssrc24 = 0x%x ssrc25 = 0x%x ssrc26 = 0x%x ssrc27 = 0x%x ssrc28 = 0x%x ssrc29 = 0x%x\n",
//      						  ssrc[24], ssrc[25], ssrc[26], ssrc[27], ssrc[28], ssrc[29]); 
						} 
					    else if (dspl_format == 'F') { // display short data
							pcorrect = 1.0; //(float)fifopiraq1->data.info.hits; //pow(10., 0.1*fifopiraq1->data.info.data_sys_sat);
							fsrc = (float *)fifopiraq1->data.data;
							gates_gg = (int)fifopiraq1->data.info.gates*6-6;
//printf("0:TOTALSIZE = %d fsrc[0] = %+8.3f fsrc[1] = %+8.3f fsrc[2] = %+8.3f fsrc[%d] = %+8.3f testnum = %d\n", TOTALSIZE(fifopiraq1), fsrc[0], fsrc[1], fsrc[2], (2*fifopiraq1->data.info.gates)-1, fsrc[(2*fifopiraq1->data.info.gates)-1], testnum); 
//printf("\n"); 
//printf("fsrc2[5] = %+8.3f\n", *fsrc2); 
//printf("0:TOTALSIZE = %d CurPkt hits = %d CurPkt channel = %d testnum = %d\n", TOTALSIZE(fifopiraq1), fifopiraq1->data.info.clutter_start[0], fifopiraq1->data.info.clutter_end[0], testnum); 
//printf("                 ts_start = %d ts_end = %d\n", fifopiraq1->data.info.ts_start_gate, fifopiraq1->data.info.ts_end_gate); 
//printf("                 az = %4.3f el = %4.3f\n",fifopiraq1->data.info.az,fifopiraq1->data.info.el); 
//							printf("Saturation %8.2f, Pcorrect %8.3f\n", fifopiraq1->data.info.data_sys_sat, pcorrect);
//							printf("Gate %04d:A0 = %+8.3e B0 = %+8.3e P0 = %+8.3e \n          A1 = %+8.3e B1 = %+8.3e P1 = %+8.3e\n",
//			  		  0, fsrc[0], fsrc[1], fsrc[2]*pcorrect, fsrc[3], fsrc[4], fsrc[5]*pcorrect); 
							}
						} // end	if ((testnum % ...
//!!!					printf("fifo1_hits = %d\n",fifo1_hits); 
					testnum++; fifo1_hits++;   
#ifndef DRX_PACKET_TESTING	// define activates data packet resizing for CP2 throughput testing. 
					fifopiraq1->udp.totalsize = TOTALSIZE(fifopiraq1); // ordinary operation
#else	// increase udp-send totalsize to Nhits
					int test_totalsize1 = Nhits*(TOTALSIZE(fifopiraq1) + BUFFER_EPSILON); 
					fifopiraq1->udp.totalsize = test_totalsize1; // CP2 throughput testing
//printf("0:udpTOTALSIZE = %d\n", test_totalsize1); 
#endif
#if 1	// CP2: data packets sized at runtime; test w/smaller packet first
					seq1 = send_udp_packet(outsock, outport, seq1, udp1); 
#endif
					fifo_increment_tail(fifo1);
				} // end	while(fifo_hit()
				cycle_fifo1_hits = 0; // clear cycle counter 
			} // end	else if(val == 0) 
#if 0 // 
			if (fakeangles == FALSE) 
				pmac_WD6_acquire(fifopiraq1, FALSE); // acquire PMAC data w/o interpolation
#endif
		}
// piraq2: 
		if (piraqs & 0x02) { // turn on slot 2
select2:
			tv2.tv_sec = 0;
			tv2.tv_usec = 1000; 
			/* do polling */
			FD_ZERO(&rfd2);
			FD_SET(cmd2_notifysock, &rfd2); //!!!Mitch eliminated but while(1) below depends on it.  
			val2 = select(cmd2_notifysock + 1, &rfd2, 0, 0, &tv2);
			if (val2 < 0) 
				{perror( "select" );  continue;}

			else if(val2 == 0) { /* use time out for polling */
//printf("1:cur_fifo2_hits = %d\n", cur_fifo2_hits); 
				// PIRAQ2:
				// take CYCLE_HITS beams from piraq:
					while(((cur_fifo2_hits = fifo_hit(fifo2)) > 0) && (cycle_fifo2_hits < CYCLE_HITS)) { // fifo1 hits ready: save #hits pending 
if ((cur_fifo2_hits % 5) == 0) { printf("b1: hits2 = %d\n", cur_fifo2_hits); } // print often enough ... 
						cycle_fifo2_hits++; 
					fifopiraq2 = (PACKET *)fifo_get_read_address(fifo2,0);
					if (fakeangles == FALSE) {
						r_c = pmac_WD6_acquire(fifopiraq2, TRUE); 
						if (r_c == FALSE) { printf("pmac_WD6_acquire() r_c = FALSE\n"); break; } // interpolation second linear point not yet available 
					}
#ifdef EOF_DETECT
					if ((int)fifopiraq2->data.info.packetflag == -1) { // piraq detected a hardware out-of-sync condition "EOF" 
						printf("\n\npiraq2 EOF detected. exiting.\n"); 
						if (piraqs & 0x01) { // slot 1 active
							stop_piraq(config, piraq1); 
						} 
						printf("\n\npiraq1 stopped\n"); 
						if (piraqs & 0x02) { // slot 2 active
							stop_piraq(config, piraq2); 
						} 
						if (piraqs & 0x04) { // slot 3 active
							stop_piraq(config, piraq3); 
						} 
						printf("\n\npiraqs stopped\n"); 
#ifdef TIMER_CARD
						timer_stop(&ext_timer); 
#endif
						delete piraq1; delete piraq2; delete piraq3; 
						printf("\n\ngoing to eof_start_over\n"); 
						goto eof_start_over; 
					}
#endif // end #ifdef EOF_DETECT
					hits2 = fifopiraq2->data.info.hits; gates2 = fifopiraq2->data.info.gates; 
					scale2 = (float)(PIRAQ3D_SCALE*PIRAQ3D_SCALE*hits2); // scale fifo2 data
					udp2 = &fifopiraq2->udp;
					udp2->magic = MAGIC;
					if (config->dataformat == PIRAQ_ABPDATA) { // PIRAQ_ABPDATA: RAPID DOW
						udp2->type = UDPTYPE_PIRAQ_ABPDATA; 
						fifopiraq2->data.info.bytespergate = 6 * (sizeof(float)); // GRG ABPDATA
						// scale ABPs: 
						fsrc = (float *)fifopiraq2->data.data; 
#ifdef	DRX_PACKET_TESTING	// define to activate data packet resizing for throughput testing. 
//						for (i = 0; i < 6*gates2 + 6*fifopiraq2->data.info.clutter_end[0]*fifopiraq2->data.info.gates; i++) {	// vary multiplier; note CPU usage in Task Manager. 6 = data/gate
//							*fsrc *= scale2; fsrc++;		// scale the data using multiplication
//						}
#else
						for (i = 0; i < 6*gates2; i++) {	// 6 data/gate
							*fsrc /= scale2; fsrc++;		// scale the data
						}
#endif
						if ((int)fifopiraq2->data.info.ts_start_gate != -1) { // diagnostic timeseries appended 
							int tsgates2 = (fifopiraq2->data.info.ts_end_gate - fifopiraq2->data.info.ts_start_gate) + 1; 
							for (i = 0; i < 4*hits2*tsgates2; i++) {	// 6 data/gate
								*fsrc /= PIRAQ3D_SCALE; fsrc++;		// scale the data -- 1 hit
							} 
						}
					}
					else if (config->dataformat == PIRAQ_ABPDATA_STAGGER_PRT) { // PIRAQ_ABPDATA w/Staggered PRT 
						udp2->type = UDPTYPE_PIRAQ_ABPDATA_STAGGER_PRT; 
						fifopiraq2->data.info.bytespergate = 12 * (sizeof(float)); // Staggered PRT ABPDATA
						fsrc = (float *)fifopiraq2->data.data; 
						for (i = 0; i < 12*gates2; i++) {	// 6 data/gate, twice
							*fsrc /= scale2; fsrc++;		// scale the data
						}
						if ((int)fifopiraq2->data.info.ts_start_gate != -1) { // diagnostic timeseries appended 
							int tsgates2 = (fifopiraq2->data.info.ts_end_gate - fifopiraq2->data.info.ts_start_gate) + 1; 
							for (i = 0; i < 4*hits2*tsgates2; i++) {	// 6 data/gate
								*fsrc /= PIRAQ3D_SCALE; fsrc++;		// scale the data -- 1 hit
							} 
						}
					}
					else if (config->dataformat == PIRAQ_CP2_TIMESERIES) { // CP2 Timeseries 
						udp2->type = UDPTYPE_PIRAQ_CP2_TIMESERIES; 
						fifopiraq2->data.info.bytespergate = 2 * (sizeof(float)); // Staggered PRT ABPDATA
						fsrc = (float *)fifopiraq2->data.data; 
						// no data scaling
					}
					else {                 // short-integer IQDATA: time series
						udp2->type = UDPTYPE_IQDATA; 
						fifopiraq2->data.info.bytespergate = 4 * (sizeof(short)); // IQDATA 
					}
					temp2 = fifopiraq2->data.info.pulse_num * (unsigned __int64)(prt * (float)COUNTFREQ + 0.5);
					fifopiraq2->data.info.secs = temp2 / COUNTFREQ;
					fifopiraq2->data.info.nanosecs = ((unsigned __int64)10000 * (temp2 % ((unsigned __int64)COUNTFREQ))) / (unsigned __int64)COUNTFREQ;
					fifopiraq2->data.info.nanosecs *= (unsigned __int64)100000; // multiply by 10**5 to get nanoseconds
					
					if (fakeangles == TRUE) { // no PMAC: fakin it from here ...  
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
					} 
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
#if 1	// test N-hit packet data: test sequential PNs 
				__int64 * __int64_ptr, * __int64_ptr2; unsigned int * uint_ptr; float * fsrc2; 
				for (i = 0; i < Nhits; i++) { // all hits in the packet 
//				for (i = 0; i < Nhits+1; i++) { // all hits in the packet + 1
//				for (i = 0; i < 1; i++) { // first few hits in the packet 
					// compute pointer to datum in an individual hit, dereference and print. 
					// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON)
					__int64_ptr = (__int64 *)((char *)&fifopiraq2->data.info.beam_num + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					beamnum = *__int64_ptr; 
					__int64_ptr2 = (__int64 *)((char *)&fifopiraq2->data.info.pulse_num + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					pulsenum = *__int64_ptr2; 
					uint_ptr = (unsigned int *)((char *)&fifopiraq2->data.info.hits + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					j = *uint_ptr; 
					uint_ptr = (unsigned int *)((char *)&fifopiraq2->data.info.channel + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					k = *uint_ptr; 
					fsrc2 = (float *)((char *)fifopiraq2->data.data + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON)));
					fsrc2 += 0; // look at THIS datum
					if (lastpulsenumber2 != pulsenum - 1) { // PNs not sequential
						printf("hit%d: lastPN = %I64d PN = %I64d\n", i+1, lastpulsenumber2, pulsenum); 
						fprintf(db_fp, "%d:hit%d: lastPN = %I64d PN = %I64d\n", fifopiraq2->data.info.channel, i+1, lastpulsenumber2, pulsenum); 
					} 
					lastpulsenumber2 = pulsenum; 
//					printf("hit%d: BN = %I64d PN = %I64d hits = %d gates = %d fsrc2[6] = %+8.3f\n", i+1, beamnum, pulsenum, j, k, *fsrc2); 
//					printf("%d: hit%02d: BN = %I64d PN = %I64d fsrc2[0] = %+8.3f fsrc2[1] = %+8.3f fsrc2[2] = %+8.3f fsrc2[end] = %+8.3f\n", k, i+1, beamnum, pulsenum, *fsrc2, fsrc2[1], fsrc2[2], fsrc2[(2*fifopiraq2->data.info.gates)-1]); 
				}
//				printf("\n"); 
#endif
						if ((testnum % dspl_hits) == 0) { // done dspl_hit cycles
						if(dspl_format != 'A') {
//						    printf("board = %d:\n", fifopiraq2->data.info.channel);
						} // end	if(dspl_format != 'A')
					    if (dspl_format == 'S') { // display short data 
							ssrc = (unsigned short *)fifopiraq2->data.data; 
							printf("ssrc0 = 0x%x\n",&ssrc[0]); 
					        printf("ssrc0 = 0x%x ssrc1 = 0x%x ssrc2 = 0x%x ssrc3 = 0x%x ssrc4 = 0x%x ssrc5 = 0x%x ssrc6 = 0x%x ssrc7 = 0x%x\n",
				  			  ssrc[0], ssrc[1], ssrc[2], ssrc[3], ssrc[4], ssrc[5], ssrc[6], ssrc[7]); 
//						    printf("ssrc24 = 0x%x ssrc25 = 0x%x ssrc26 = 0x%x ssrc27 = 0x%x ssrc28 = 0x%x ssrc29 = 0x%x\n",
//      						  ssrc[24], ssrc[25], ssrc[26], ssrc[27], ssrc[28], ssrc[29]); 
						} 
					    else if (dspl_format == 'F') { // display short data 
							fsrc = (float *)fifopiraq2->data.data;
							gates_gg = (int)fifopiraq2->data.info.gates*6-6;

							if (config->ts_start_gate >= 0) { 
								diagiqsrc = (float *)(((unsigned char *)fifopiraq2->data.data) + fifopiraq2->data.info.gates*fifopiraq2->data.info.bytespergate*sizeof(float));  
//								printf("cmd.arg[0..4] 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\n",
//										fifopiraq2->cmd.arg[0], fifopiraq2->cmd.arg[1], fifopiraq2->cmd.arg[2],
//										fifopiraq2->cmd.arg[3], fifopiraq2->cmd.arg[4]);
//								printf("testiqsrc0= 0x%x testiqsrc1= 0x%x testiqsrc2= 0x%x \ntestiqsrc3= 0x%x testiqsrc4= 0x%x testiqsrc5= 0x%x\n",
//						      		  testiqsrc[0], testiqsrc[1], testiqsrc[2], testiqsrc[3], testiqsrc[4], testiqsrc[5]); 
							}
						} 
					} // end	if ((testnum % ...
//!!!					printf("fifo2_hits = %d\n",fifo2_hits); 
					testnum++; fifo2_hits++;  
#ifndef DRX_PACKET_TESTING	// define activates data packet resizing for CP2 throughput testing. 
					fifopiraq2->udp.totalsize = TOTALSIZE(fifopiraq2); // ordinary operation
#else	// increase udp-send totalsize to Nhits
					int test_totalsize2 = Nhits*(TOTALSIZE(fifopiraq2) + BUFFER_EPSILON); // 
					fifopiraq2->udp.totalsize = test_totalsize2; // CP2 throughput testing
#endif

#if 1	// CP2: data packets sized at runtime; test w/smaller packet first
					seq2 = send_udp_packet(outsock, outport, seq2, udp2);  
#endif
					fifo_increment_tail(fifo2);
				} // end	while(fifo_hit()
				cycle_fifo2_hits = 0; 
			} // end	else if(val == 0) 
#if 0 // 
			if (fakeangles == FALSE) 
				pmac_WD6_acquire(fifopiraq2, FALSE); // acquire PMAC data w/o interpolation
#endif
		}// end piraq2	
// piraq3: 
		if (piraqs & 0x04) { // turn on slot 3
select3:
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
#if 0
printf("piraq3:\n"); piraq3->ReadEPROM(EPROM3);
for(y = 0; y < 16; y++) {
	printf("%08lX %08lX %08lX %08lX\n",EPROM3[y*4],EPROM3[y*4+1],EPROM3[y*4+2],EPROM3[y*4+3]); 
}
exit(0); 
#endif
					fifopiraq3 = (PACKET *)fifo_get_read_address(fifo3,0);
					if (fakeangles == FALSE) {
						r_c = pmac_WD6_acquire(fifopiraq3, TRUE); 
						if (r_c == FALSE) { printf("pmac_WD6_acquire() r_c = FALSE\n"); break; } // interpolation second linear point not yet available 
					}
#ifdef EOF_DETECT
					if ((int)fifopiraq3->data.info.packetflag == -1) { // piraq detected a hardware out-of-sync condition "EOF" 
						printf("\n\npiraq3 EOF detected. exiting.\n"); 
						if (piraqs & 0x01) { // slot 1 active
							stop_piraq(config, piraq1); 
						} 
						printf("\n\npiraq1 stopped\n"); 
						if (piraqs & 0x02) { // slot 2 active
							stop_piraq(config, piraq2); 
						} 
						if (piraqs & 0x04) { // slot 3 active
							stop_piraq(config, piraq3); 
						} 
						printf("\n\npiraqs stopped\n"); 
#ifdef TIMER_CARD
						timer_stop(&ext_timer); 
#endif
						delete piraq1; delete piraq2; delete piraq3; 
						printf("\n\ngoing to eof_start_over\n"); 
						goto eof_start_over; 
					}
#endif // end #ifdef EOF_DETECT
					hits3 = fifopiraq3->data.info.hits; gates3 = fifopiraq3->data.info.gates; 
					scale3 = (float)(PIRAQ3D_SCALE*PIRAQ3D_SCALE*hits3); // scale fifo3 data
					udp3 = &fifopiraq3->udp;
					udp3->magic = MAGIC;
					if (config->dataformat == PIRAQ_ABPDATA) { // PIRAQ_ABPDATA: RAPID DOW
						udp3->type = UDPTYPE_PIRAQ_ABPDATA; 
						fifopiraq3->data.info.bytespergate = 6 * (sizeof(float)); // GRG ABPDATA
						// scale ABPs: 
						fsrc = (float *)fifopiraq3->data.data; 
#ifdef	DRX_PACKET_TESTING	// define to activate data packet resizing for throughput testing. 
//						for (i = 0; i < 6*gates3 + 6*fifopiraq3->data.info.clutter_end[0]*fifopiraq3->data.info.gates; i++) {	// vary multiplier; note CPU usage in Task Manager. 6 = data/gate
//							*fsrc *= scale3; fsrc++;		// scale the data using multiplication
//						}
#else
						for (i = 0; i < 6*gates3; i++) {	// 6 data/gate
							*fsrc /= scale3; fsrc++;		// scale the data
						}
#endif
						if ((int)fifopiraq3->data.info.ts_start_gate != -1) { // diagnostic timeseries appended 
							int tsgates3 = (fifopiraq3->data.info.ts_end_gate - fifopiraq3->data.info.ts_start_gate) + 1; 
							for (i = 0; i < 4*hits3*tsgates3; i++) {	// 6 data/gate
								*fsrc /= PIRAQ3D_SCALE; fsrc++;		// scale the data -- 1 hit
							} 
						}
					}
					else if (config->dataformat == PIRAQ_ABPDATA_STAGGER_PRT) { // PIRAQ_ABPDATA w/Staggered PRT 
						udp3->type = UDPTYPE_PIRAQ_ABPDATA_STAGGER_PRT; 
						fifopiraq3->data.info.bytespergate = 12 * (sizeof(float)); // Staggered PRT ABPDATA
						fsrc = (float *)fifopiraq3->data.data; 
						for (i = 0; i < 12*gates3; i++) {	// 6 data/gate, twice
							*fsrc /= scale3; fsrc++;		// scale the data
						}
						if ((int)fifopiraq3->data.info.ts_start_gate != -1) { // diagnostic timeseries appended 
							int tsgates3 = (fifopiraq3->data.info.ts_end_gate - fifopiraq3->data.info.ts_start_gate) + 1; 
							for (i = 0; i < 4*hits3*tsgates3; i++) {	// 6 data/gate
								*fsrc /= PIRAQ3D_SCALE; fsrc++;		// scale the data -- 1 hit
							} 
						}
					}
					else if (config->dataformat == PIRAQ_CP2_TIMESERIES) { // CP2 Timeseries 
						udp3->type = UDPTYPE_PIRAQ_CP2_TIMESERIES; 
						fifopiraq3->data.info.bytespergate = 2 * (sizeof(float)); // Staggered PRT ABPDATA
						fsrc = (float *)fifopiraq3->data.data; 
						// no data scaling
					}
					else {                 // short-integer IQDATA: time series
						udp3->type = UDPTYPE_IQDATA; 
						fifopiraq3->data.info.bytespergate = 4 * (sizeof(short)); // IQDATA 
					} 
//!udp->totalsize set was up here, all 3 boards:					udp3->totalsize = TOTALSIZE(fifopiraq3);

					temp3 = fifopiraq3->data.info.pulse_num * (unsigned __int64)(prt * (float)COUNTFREQ + 0.5);
					fifopiraq3->data.info.secs = temp3 / COUNTFREQ;
					fifopiraq3->data.info.nanosecs = (10000 * (temp3 % COUNTFREQ)) / COUNTFREQ;
					fifopiraq3->data.info.nanosecs = ((unsigned __int64)10000 * (temp3 % ((unsigned __int64)COUNTFREQ))) / (unsigned __int64)COUNTFREQ;
					fifopiraq3->data.info.nanosecs *= (unsigned __int64)100000; // multiply by 10**5 to get nanoseconds
					
					if (fakeangles == TRUE) { // no PMAC: fakin it from here ...  
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
					} 
					// ... to here 
//						printf("3: az = %8.2f el = %8.2f scan_type = %d sweep = %d volume = %d\n",PMAC_az, PMAC_el, PMAC_scan_type, PMAC_sweep, PMAC_volume); 
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
#if 1	// test N-hit packet: 0: reduce printing
				__int64 * __int64_ptr, * __int64_ptr2; unsigned int * uint_ptr; float * fsrc2; 
				for (i = 0; i < Nhits; i++) { // all hits in the packet 
//				for (i = 0; i < Nhits-6; i++) { // first few hits in the packet 
//				for (i = 0; i < 4; i++) { // first few hits in the packet 
					// compute pointer to datum in an individual hit, dereference and print. 
					// CP2 PCI Bus transfer size: Nhits * (HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON)
					__int64_ptr = (__int64 *)((char *)&fifopiraq3->data.info.beam_num + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					beamnum = *__int64_ptr; 
					__int64_ptr2 = (__int64 *)((char *)&fifopiraq3->data.info.pulse_num + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					pulsenum = *__int64_ptr2; 
					uint_ptr = (unsigned int *)((char *)&fifopiraq3->data.info.hits + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					j = *uint_ptr; 
					uint_ptr = (unsigned int *)((char *)&fifopiraq3->data.info.channel + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON))); 
					k = *uint_ptr; 
					fsrc2 = (float *)((char *)fifopiraq3->data.data + i*((HEADERSIZE + (config->gatesa * bytespergate) + BUFFER_EPSILON)));
					fsrc2 += 0; // look at THIS datum
					if (lastpulsenumber3 != pulsenum - 1) { // PNs not sequential
						printf("hit%d: lastPN = %I64d PN = %I64d\n", i+1, lastpulsenumber3, pulsenum); 
						fprintf(db_fp, "%d:hit%d: lastPN = %I64d PN = %I64d\n", fifopiraq3->data.info.channel, i+1, lastpulsenumber3, pulsenum); 
					} 
					lastpulsenumber3 = pulsenum; // previous hit PN
//					printf("hit%d: BN = %I64d PN = %I64d hits = %d gates = %d fsrc2[6] = %+8.3f\n", i+1, beamnum, pulsenum, j, k, *fsrc2); 
//					printf("%d: hit%02d: BN = %I64d PN = %I64d fsrc2[0] = %+8.3f fsrc2[1] = %+8.3f fsrc2[2] = %+8.3f fsrc2[end] = %+8.3f\n", k, i+1, beamnum, pulsenum, *fsrc2, fsrc2[1], fsrc2[2], fsrc2[(2*fifopiraq3->data.info.gates)-1]); 
//					printf("hit%d: &BN = 0x%x\n", i+1, __int64_ptr); 
				}
//				printf("\n"); 
#endif
						if ((testnum % dspl_hits) == 0) { // done dspl_hit cycles
						if(dspl_format != 'A') {
//						    printf("board = %d:\n", fifopiraq3->data.info.channel);
						} // end	if(dspl_format != 'A')
					    if (dspl_format == 'S') { // display short data 
							ssrc = (unsigned short *)fifopiraq3->data.data; 
							printf("ssrc0 = 0x%x\n",&ssrc[0]); 
					        printf("ssrc0 = 0x%x ssrc1 = 0x%x ssrc2 = 0x%x ssrc3 = 0x%x ssrc4 = 0x%x ssrc5 = 0x%x ssrc6 = 0x%x ssrc7 = 0x%x\n",
				  			  ssrc[0], ssrc[1], ssrc[2], ssrc[3], ssrc[4], ssrc[5], ssrc[6], ssrc[7]); 
//						    printf("ssrc24 = 0x%x ssrc25 = 0x%x ssrc26 = 0x%x ssrc27 = 0x%x ssrc28 = 0x%x ssrc29 = 0x%x\n",
//      						  ssrc[24], ssrc[25], ssrc[26], ssrc[27], ssrc[28], ssrc[29]); 
						} 
					    else if (dspl_format == 'F') { // display short data 
							fsrc = (float *)fifopiraq3->data.data;
							gates_gg = (int)fifopiraq3->data.info.gates*6-6;
//printf("2:TOTALSIZE = %d testnum = %d\n", TOTALSIZE(fifopiraq3), testnum); 
//printf("                 az = %4.3f el = %4.3f\n",fifopiraq3->data.info.az,fifopiraq3->data.info.el); 
//					        printf("Gate %04d:A0 = %+8.3e B0 = %+8.3e P0 = %+8.3e \n          A1 = %+8.3e B1 = %+8.3e P1 = %+8.3e\n",
//					  		  0, fsrc[0], fsrc[1], fsrc[2], fsrc[3], fsrc[4], fsrc[5]); 
						if (config->ts_start_gate >= 0) { 
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
					if ((testnum % 100) == 0) {printf("test_totalsize3 = %d TOTALSIZE = %d\n", test_totalsize3, TOTALSIZE(fifopiraq3)); } 
#endif
//fifopiraq3->udp.totalsize = TOTALSIZE(fifopiraq3); // ordinary operation
#if 1	// CP2: data packets sized at runtime; test w/smaller packet first
					seq3 = send_udp_packet(outsock, outport, seq3, udp3);  
#endif
					fifo_increment_tail(fifo3);
				} // end	while(fifo_hit()
				cycle_fifo3_hits = 0; 
			} // end	else if(val == 0) 
#if 0 // 
			if (fakeangles == FALSE) 
				pmac_WD6_acquire(fifopiraq3, FALSE); // acquire PMAC data w/o interpolation
#endif
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
				if(c == 'F')		fakeangles ^= 1;
				// !!!DO LATER: leave these here but do not use for now. 
//				if(c == '+')		TimerStartCorrection += (__int64)ExactmSecperBeam; 
//				if(c == '-')		TimerStartCorrection -= (__int64)ExactmSecperBeam; 
#ifndef TESTING_TIMESERIES
				if(c == '+')		TimerStartCorrection += (__int64)mSecperBeam; 
				if(c == '-')		TimerStartCorrection -= (__int64)mSecperBeam; 
#else
				// adjust test timeseries power: !note requires NOT TESTING_TIMESERIES_RANGE. 
				if(c == 'U')		test_ts_power *= sqrt(10.0); 
				if(c == 'D')		test_ts_power /= sqrt(10.0); // factor of arg in power
				// adjust test timeseries frequency: 
				if(c == '+')		test_ts_adjust += 2; 
				if(c == '-')		test_ts_adjust -= 2; 
#endif

				if(c == 'S')		send ^= 1;
				if(c == 'Q' || c == 27)   {printf("\nUser terminated:\n");	break;}
			} // end if (kbhit())  
		} // end	while(1)

// exit NOW:		printf("\npress a key to stop piraqs and exit\n"); while(!keyvalid()) ; 
		printf("piraqs stopped.\nexit.\n\n"); 
leave: 
		if (piraqs & 0x01) { // slot 1 active
			stop_piraq(config, piraq1); 
		} 
		if (piraqs & 0x02) { // slot 2 active
			stop_piraq(config, piraq2); 
		} 
		if (piraqs & 0x04) { // slot 3 active
			stop_piraq(config, piraq3); 
		} 
#ifdef TIMER_CARD
// remove for lab testing: keep transmitter pulses active w/o go.exe running. 12-9-04
		timer_stop(&ext_timer); 
#endif
		delete piraq1; 
		delete piraq2; 
		delete piraq3;
		fclose(db_fp);
		printf("TimerStartCorrection = %dmSec\n", TimerStartCorrection); 
		exit(0); 

	} // end if (piraqs)
leave_now: 
	stop_piraq(config, piraq);	
#ifdef TIMER_CARD
	timer_stop(&ext_timer); 
#endif
	delete piraq; 
	printf("\n%d: fifo_hits = %d\n", testnum, testnum); 
	exit(0); 
}

// array of structures containing PMAC DPRAM data and associated quantities required to interpolate angle 
// from packet beamnumber
#define PMACDPRAMDataSize 2048
struct {
	__int64  acquire_time; // time at end of acquisition interval 
	float az_avg; // average azimuth, this interval 
	float el_avg; // average elevation
	unsigned short scan_type; 
	unsigned short sweep; 
	unsigned short volume; 
	unsigned short whatsit; 
	unsigned short transition; 
	unsigned short points; // #samples this average
	float avg_az_sweep_rate; // sweep rate, over previous ~200mSec 
	float avg_el_sweep_rate; // sweep rate, over previous ~200mSec 
} PMACDPRAMData[PMACDPRAMDataSize]; 

// PMAC stuff 
#define INTERP_START_LATENCY	250		// milliseconds before angle data sufficient to begin interpolation 
#define LATENCY_SWEEP_RATE		0.001	// very small
//#define ANGLE_AVERAGE_MSEC		200		// interval for computing average sweep rate for az, el 
#define SWEEP_AVERAGE_MSEC		 70		// interval for computing average sweep rate for az, el 
#define FIRST_INTERP_STEP		12		// step back in PMAC timeseries buffer to get 1st interpolation data point 
static  unsigned short    *Dpram=0;       // for reading PMAC 
static PMAC_HANDLE pmac_handl; 
static float az, el;
static float last_az, delta_az; 
static float az_sum, az_avg, az_interp; 
static float delta_el; 
static float el_sum, el_avg, el_interp; 
static float last_ai, last_ei; // previous interpolated angles and diffs
static float daI, deI; 
static unsigned int angle_samples = 0; // total samples taken
static _int64 BeginEpochMillisec; // start of PMAC data acquisition 
static _int64 SearchEpochMillisec; // start of PMAC data acquisition 
static unsigned int PMACDPRAMDataidx = 0; 
static int PMACDPRAMDataFirstTime = TRUE; 
static int SearchPMACDPRAMDataidx = 0; // search forward from previous find; begin at zero 
static int SearchPMACDPRAMDataacqidx; 
static int FirstPMACDPRAMInterpidx, SecondPMACDPRAMInterpidx; // begin, end entries for interpolation 
static int firstInterpolation = FALSE; 

struct _timeb PMACtimebuffer;
int b0 = 0; // diagnostic beam accumulations; printed and cleared once per second. 
int b1 = 0; 
int b2 = 0; 
// !note: if interpolate == FALSE, pkt not valid so do not use it. 
int pmac_WD6_acquire(PACKET * pkt, int interpolate) { 
	int  size;
	unsigned short * bp_data;
	unsigned short scan_type, sweep, volume, transition; 
	static int i =0;
	static int n =0;

// method using pointer to PMAC DPRAM from WD6, set in PIRAQ mailbox3: 
//az = (*(PMACDPRAM)) * 360/65536.0; 
//printf("PIRAQ pointer az = %5.2f\n",az);

// method using Windriver6:    
//    Open the PMAC: 
//    if(!pmac_handl)
//		PMAC_Open(&pmac_handl,PMAC_DEFAULT_VENDOR_ID,PMAC_DEFAULT_DEVICE_ID,0);
      
//   dwell->header.az = (PMAC_ReadWord(pmac,0,0)) * 360/65536.0;//i++%360;//Dpram[0] * 360.0 / 65536.0;
//   dwell->header.el = (PMAC_ReadWord(pmac,0,2)) * 360/65536.0;//i++%360;//Dpram[1] * 360.0 / 65536.0;

//   az = (PMAC_ReadWord(pmac,0,0)) * 360/65536.0; 
//printf("PMAC_ReadWord az = %5.2f\n",az);

	if (PMACDPRAMDataFirstTime == TRUE) { // first PMAC DPRAM data acquisition
		_ftime( &PMACtimebuffer ); millisec = PMACtimebuffer.millitm; 
		BeginEpochMillisec = (PMACtimebuffer.time * (__int64)1000) + (__int64)PMACtimebuffer.millitm; 
		printf("BeginEpochMillisec =  %I64d\n", BeginEpochMillisec); 
		PMACDPRAMDataFirstTime = FALSE; 
	}
#if 1 // turn on to get az displayed direct from PMAC
//bp_data = (unsigned short *)PMAC_GetBasePtr(pmac, 0); // az offset 0

	az = (PMAC_ReadWord(pmac,0,0)) * 360/65536.0;  
//printf("PMAC = 0x%x az = %5.2f laz = %5.2f\n", PMAC_ReadWord(pmac,0,0), az, last_az);
//printf("n = %d after 1 PMAC_ReadWord(pmac,0,0)\n", n); n++; 
	el = (PMAC_ReadWord(pmac,0,2)) * 360/65536.0;
//printf("n = %d\n", n); n++; 
 
//az = bp_data[0] * (360/65536.0); 
//printf("bp_data az = %5.2f\n",az);
#endif
	scan_type = PMAC_ReadWord(pmac,0,4);
	sweep = PMAC_ReadWord(pmac,0,6);
	volume = PMAC_ReadWord(pmac,0,8);
	size = PMAC_ReadWord(pmac,0,10);
	transition = PMAC_ReadWord(pmac,0,12);
	// for mSec-resolution time tests: 
	_ftime( &PMACtimebuffer ); millisec = PMACtimebuffer.millitm; 
	if (interpolate == TRUE) {
		RadarEpochMillisec = (__int64)(fpExactmSecperBeam * ((double)pkt->data.info.beam_num) + fpRadarSystemCorrection); // time packet acquired, by beamnumber 
//!!!		RadarEpochMillisec = ((__int64)mSecperBeam) * pkt->data.info.beam_num; // time packet acquired, by beamnumber 
		//printf("n = %d interpolate == TRUE\n", n); n++; 
		RadarEpochMillisec += (__int64)(TimerStartCorrection); // compensate for timer board startup interval
if (pkt->data.info.channel == 0) // accumulate beam count for each board 
b0++; 
else if (pkt->data.info.channel == 1) 
b1++; 
else if (pkt->data.info.channel == 2)
b2++; 
//printf("n = %d interpolate == TRUE\n", n); n++; 
	}  
//printf("RadarEpochMillisec = %I64d \nSystemEpochMillisec = %I64d\n", RadarEpochMillisec, SystemEpochMillisec); 
//	timeline = ctime( & ( timebuffer.time ) );
//	if (((az - last_az) > 90.0) || ((az - last_az) < -90.0))
//		printf("az = %4.3f last_az = %4.3f\n", az, last_az); 

// print board# here: 
//printf( "b=%d:\nS=%I64d mSec=%hu last_mSec=%hu d_mSec=%d az_avg=%4.3f el_avg=%4.3f n=%d idx=%d\nR=%I64d BN=%I64d\n", pkt->data.info.channel, SystemEpochMillisec, timebuffer.millitm, last_millisec, delta_millisec, az_avg, el_avg, PMAC_acquire_times, PMACDPRAMDataidx, RadarEpochMillisec, pkt->data.info.beam_num); 
// print board# above: 
//printf( "S=%I64d mSec=%hu last_mSec=%hu d_mSec=%d az_avg=%4.3f el_avg=%4.3f n=%d idx=%d\nR=%I64d BN=%I64d\n", SystemEpochMillisec, timebuffer.millitm, last_millisec, delta_millisec, az_avg, el_avg, PMAC_acquire_times, PMACDPRAMDataidx, RadarEpochMillisec, pkt->data.info.beam_num); 
	
	if (last_millisec != millisec) { 
//printf("n = %d last_millisec != millisec\n", n); n++; 
//		printf( "az = %5.2f el = %5.2f scan=%d sweep=%d volume=%d transition=%d mSec=%hu last_mSec=%hu n=%d\n", az, el, scan_type, sweep, volume, transition, timebuffer.millitm, last_millisec, PMAC_acquire_times); 
//		printf( "az = %5.2f el = %5.2f scan=%d sweep=%d volume=%d transition=%d mSec=%hu last_mSec=%hu n=%d\n", az, el, scan_type, sweep, volume, transition, timebuffer.millitm, last_millisec, PMAC_acquire_times); 
		if (millisec > last_millisec) 
			delta_millisec = millisec - last_millisec; 
		else { // system seconds rollover
			delta_millisec = (millisec + 1000) - last_millisec; 
printf("beam count: b0 = %d b1 = %d b2 = %d\n", b0, b1, b2); b0 = b1 = b2 = 0; // print accumulations; clear 

		}
//printf("n = %d last_millisec != millisec\n", n); n++; 
		if (interpolate == TRUE) {
			RadarEpochMillisec = (__int64)(fpExactmSecperBeam * ((double)pkt->data.info.beam_num) + fpRadarSystemCorrection); // time packet acquired, by beamnumber 
//!!!			RadarEpochMillisec = ((__int64)mSecperBeam) * pkt->data.info.beam_num; // time packet acquired, by beamnumber 

//printf("n = %d last_millisec != millisec, interp == TRUE\n", n); n++; 
			RadarEpochMillisec += (__int64)(TimerStartCorrection); // compensate for timer board startup interval
//printf("n = %d last_millisec != millisec, interp == TRUE\n", n); n++; 
		}
//		if (interpolate == FALSE) { // acquire angles only
//			printf("NO INTERPOLATION:\n"); 
//		}
//SystemEpochMillisec = (timebuffer.time * (__int64)1000) + (__int64)timebuffer.millitm; // take this LATER: see notes 5-10-04
		az_avg = az_sum / PMAC_acquire_times; // average last set, both angles
//printf("n = %d last_millisec != millisec, / PMAC_acquire_times\n", n); n++; 
		el_avg = el_sum / PMAC_acquire_times; 
//printf("n = %d last_millisec != millisec, / PMAC_acquire_times\n", n); n++; 
//		printf( "b=%d:\nS=%I64d \nR=%I64d az = %5.2f el = %5.2f scan=%d sweep=%d volume=%d transition=%d mSec=%hu last_mSec=%hu n=%d\n", pkt->data.info.channel, SystemEpochMillisec, RadarEpochMillisec, az, el, scan_type, sweep, volume, transition, timebuffer.millitm, last_millisec, PMAC_acquire_times); 
//		printf( "BEGIN interval:\n"); // new interval; subsequent data applies to it. 
//		printf( "b=%d:\nS=%I64d \nR=%I64d mSec=%hu last_mSec=%hu d_mSec=%d az=%4.2f az_avg=%4.3f el=%4.2f el_avg=%4.3f n=%d\n\n", pkt->data.info.channel, SystemEpochMillisec, RadarEpochMillisec, timebuffer.millitm, last_millisec, delta_millisec, az, az_avg, el, el_avg, PMAC_acquire_times); 
		if (interpolate == TRUE) {
//			printf( "b=%d:\nS=%I64d mSec=%hu last_mSec=%hu d_mSec=%d az_avg=%4.3f el_avg=%4.3f n=%d idx=%d\nR=%I64d\n", pkt->data.info.channel, SystemEpochMillisec, timebuffer.millitm, last_millisec, delta_millisec, az_avg, el_avg, PMAC_acquire_times, PMACDPRAMDataidx, RadarEpochMillisec); 
//			printf( "b=%d:\nS=%I64d mSec=%hu last_mSec=%hu d_mSec=%d az_avg=%4.3f el_avg=%4.3f n=%d idx=%d\nR=%I64d BN=%I64d\n", pkt->data.info.channel, SystemEpochMillisec, timebuffer.millitm, last_millisec, delta_millisec, az_avg, el_avg, PMAC_acquire_times, PMACDPRAMDataidx, RadarEpochMillisec, pkt->data.info.beam_num); 
		} 
		else { 
//			printf( "S=%I64d mSec=%hu last_mSec=%hu d_mSec=%d az_avg=%4.3f el_avg=%4.3f n=%d idx=%d\nR=%I64d\n", SystemEpochMillisec, timebuffer.millitm, last_millisec, delta_millisec, az_avg, el_avg, PMAC_acquire_times, PMACDPRAMDataidx, RadarEpochMillisec); 
//			printf( "S=%I64d mSec=%hu last_mSec=%hu d_mSec=%d az_avg=%4.3f el_avg=%4.3f n=%d idx=%d\nR=%I64d BN=%I64d\n", SystemEpochMillisec, timebuffer.millitm, last_millisec, delta_millisec, az_avg, el_avg, PMAC_acquire_times, PMACDPRAMDataidx, RadarEpochMillisec, pkt->data.info.beam_num); 
		} 
		// in any case take another PMAC timeseries point: 
		PMACDPRAMData[PMACDPRAMDataidx].acquire_time = SystemEpochMillisec; // save PREVIOUS value; see notes 5-10-04 
		PMACDPRAMData[PMACDPRAMDataidx].az_avg = az_avg; 
		PMACDPRAMData[PMACDPRAMDataidx].el_avg = el_avg; 
		PMACDPRAMData[PMACDPRAMDataidx].scan_type = scan_type; 
		PMACDPRAMData[PMACDPRAMDataidx].sweep = sweep; // scan_num
		PMACDPRAMData[PMACDPRAMDataidx].volume = volume; // vol_num
		PMACDPRAMData[PMACDPRAMDataidx].whatsit = size; 
		PMACDPRAMData[PMACDPRAMDataidx].transition = transition; 
		PMACDPRAMData[PMACDPRAMDataidx].points = PMAC_acquire_times; 
		if (RadarEpochMillisec < (BeginEpochMillisec + INTERP_START_LATENCY)) {	// just started running: not enough data accumulated for interpolation
			PMACDPRAMData[PMACDPRAMDataidx].avg_az_sweep_rate = LATENCY_SWEEP_RATE; // fill w/small but finite value
			PMACDPRAMData[PMACDPRAMDataidx].avg_el_sweep_rate = LATENCY_SWEEP_RATE; 
		}
		else { // compute average sweep rate over previous 200mSec or so, set by #define
			SearchPMACDPRAMDataacqidx = PMACDPRAMDataidx; // start w/latest entry: inefficient, but safe
			while((SystemEpochMillisec - PMACDPRAMData[SearchPMACDPRAMDataacqidx].acquire_time) < SWEEP_AVERAGE_MSEC) { 
				// most-recent acquire - stored acquire < #define interval 
				SearchPMACDPRAMDataacqidx--; 
				if (SearchPMACDPRAMDataacqidx < 0) 
					SearchPMACDPRAMDataacqidx = PMACDPRAMDataSize - 1; 
			} 
			// back up 1 more; 1st entry > 200 mSec before
			SearchPMACDPRAMDataacqidx--; 
			if (SearchPMACDPRAMDataacqidx < 0) 
				SearchPMACDPRAMDataacqidx = PMACDPRAMDataSize - 1; 
			// compute sweep rate over this interval 
			PMACDPRAMData[PMACDPRAMDataidx].avg_az_sweep_rate = (az_avg - PMACDPRAMData[SearchPMACDPRAMDataacqidx].az_avg) / (SystemEpochMillisec - PMACDPRAMData[SearchPMACDPRAMDataacqidx].acquire_time); 
			PMACDPRAMData[PMACDPRAMDataidx].avg_el_sweep_rate = (el_avg - PMACDPRAMData[SearchPMACDPRAMDataacqidx].el_avg) / (SystemEpochMillisec - PMACDPRAMData[SearchPMACDPRAMDataacqidx].acquire_time); 
//			printf("avg az rate = %4.3f avg el rate = %4.3f\n", PMACDPRAMData[PMACDPRAMDataidx].avg_az_sweep_rate, PMACDPRAMData[PMACDPRAMDataidx].avg_el_sweep_rate); 
//			printf("avg begin at idx = %d\n", SearchPMACDPRAMDataacqidx); 
		} 
		// NOW maintain this index: 
		PMACDPRAMDataidx++; 
		if (PMACDPRAMDataidx == PMACDPRAMDataSize) // at end 
			PMACDPRAMDataidx = 0;
//printf("PMACDPRAMDataidx = %d\n", PMACDPRAMDataidx); 
		// az el mSec last_mSec times
//		printf( "%5.2f,%5.2f,%hu,%hu,n=%d\n", az, el, timebuffer.millitm, last_millisec, PMAC_acquire_times); 
		SystemEpochMillisec = (PMACtimebuffer.time * (__int64)1000) + (__int64)PMACtimebuffer.millitm; // take this NOW: see notes 5-10-04
		PMAC_acquire_times = 1; // count this one 
        az_sum = az; el_sum = el; // begin sums for averaging 
	} 
	else {
		PMAC_acquire_times++; 
        az_sum += az; el_sum += el; // continue sums for averaging 
	} 
	last_millisec = millisec; // record previous timestamp 
//	printf("b:%d az = %4.3f el = %4.3f\n", pkt->data.info.channel, az, el); 
	last_az = az; 
	if (interpolate == FALSE) { // acquire angles only
		return(FALSE); //printf("no interpolation\n"); 
	}
	if (RadarEpochMillisec < (BeginEpochMillisec + INTERP_START_LATENCY)) {	// running < 200mSec: not enough data accumulated for interpolation
		printf("insufficient angle data\n");				// !note: reduce this interval after reliable  
//printf( "S=%I64d mSec=%hu last_mSec=%hu d_mSec=%d az_avg=%4.3f el_avg=%4.3f n=%d idx=%d\nR=%I64d BN=%I64d\n", SystemEpochMillisec, timebuffer.millitm, last_millisec, delta_millisec, az_avg, el_avg, PMAC_acquire_times, PMACDPRAMDataidx, RadarEpochMillisec, pkt->data.info.beam_num); 
//		SearchPMACDPRAMDataidx = PMACDPRAMDataidx; // save last rejected; begin searches there. 
		return(TRUE); // tell main to keep getting data 
	}
	int search_times = 0; 
	// search for previous data points bracketing current packet's timestamp: 
		// search for two PMACDPRAMData[] entries bracketing RadarEpochMillisec:
		FirstPMACDPRAMInterpidx = PMACDPRAMDataidx; // index w/most recent
		FirstPMACDPRAMInterpidx--;
		if (FirstPMACDPRAMInterpidx < 0)
			FirstPMACDPRAMInterpidx = PMACDPRAMDataSize - 1; 
		// put error trap here for when RadarEpochMillisec > most-recent time 
		while(1) { // break on find of required index 
			if (PMACDPRAMData[FirstPMACDPRAMInterpidx].acquire_time > RadarEpochMillisec) { // PMAC DPRAM Data time > beam time
				FirstPMACDPRAMInterpidx--;							// index previous entry  
				if (FirstPMACDPRAMInterpidx < 0)
					FirstPMACDPRAMInterpidx = PMACDPRAMDataSize - 1; 
			} 
			else { //  got index of first acquire-time < RadarEpochMillisec 
				SecondPMACDPRAMInterpidx = FirstPMACDPRAMInterpidx;	//  interpolation data point index 
				SecondPMACDPRAMInterpidx++;							// index previous entry for 1st 
				if (SecondPMACDPRAMInterpidx >= PMACDPRAMDataSize)
					SecondPMACDPRAMInterpidx = 0; 
				break; 
			}
		}
//		goto interpolate;

interpolate: 
		if (PMACDPRAMDataidx == SecondPMACDPRAMInterpidx) { // second interpolation indexes future -- no data there -- return
//	; // !remove for now	
//printf( "S=%I64d mSec=%hu last_mSec=%hu d_mSec=%d az_avg=%4.3f el_avg=%4.3f n=%d idx=%d\nR=%I64d BN=%I64d\n", SystemEpochMillisec, timebuffer.millitm, last_millisec, delta_millisec, az_avg, el_avg, PMAC_acquire_times, PMACDPRAMDataidx, RadarEpochMillisec, pkt->data.info.beam_num); 
//printf("PMACDPRAMDataidx == SecondPMACDPRAMInterpidx\n"); 
			return(FALSE); 
		} // tell main to wait a bit ... 
//printf( "S=%I64d mSec=%hu last_mSec=%hu d_mSec=%d az_avg=%4.3f el_avg=%4.3f n=%d idx=%d\nR=%I64d BN=%I64d\n", SystemEpochMillisec, timebuffer.millitm, last_millisec, delta_millisec, az_avg, el_avg, PMAC_acquire_times, PMACDPRAMDataidx, RadarEpochMillisec, pkt->data.info.beam_num); 
//printf("Interp success.\n"); 

	float az1, az2; 
	float az1_corr, az2_corr; 
	az1_corr = 0.0; az2_corr = 0.0; 
	az1 = PMACDPRAMData[FirstPMACDPRAMInterpidx].az_avg; az2 = PMACDPRAMData[SecondPMACDPRAMInterpidx].az_avg; 
//	if (((az2 - az1) > 90.0) || ((az2 - az1) < -90.0)) // large angle discontinuity
//		printf("big leap!! az2 = %4.3f az1 = %4.3f\n", az2, az1); 
	az1 += az1_corr; az2 += az2_corr; 
//	az_interp = PMACDPRAMData[FirstPMACDPRAMInterpidx].az_avg+(RadarEpochMillisec - PMACDPRAMData[FirstPMACDPRAMInterpidx].acquire_time)*((PMACDPRAMData[SecondPMACDPRAMInterpidx].az_avg-PMACDPRAMData[FirstPMACDPRAMInterpidx].az_avg)/(PMACDPRAMData[SecondPMACDPRAMInterpidx].acquire_time-PMACDPRAMData[FirstPMACDPRAMInterpidx].acquire_time));
	az_interp = az1+(RadarEpochMillisec - PMACDPRAMData[FirstPMACDPRAMInterpidx].acquire_time)*((az2-az1)/(PMACDPRAMData[SecondPMACDPRAMInterpidx].acquire_time-PMACDPRAMData[FirstPMACDPRAMInterpidx].acquire_time));
	el_interp = PMACDPRAMData[FirstPMACDPRAMInterpidx].el_avg+(RadarEpochMillisec - PMACDPRAMData[FirstPMACDPRAMInterpidx].acquire_time)*((PMACDPRAMData[SecondPMACDPRAMInterpidx].el_avg-PMACDPRAMData[FirstPMACDPRAMInterpidx].el_avg)/(PMACDPRAMData[SecondPMACDPRAMInterpidx].acquire_time-PMACDPRAMData[FirstPMACDPRAMInterpidx].acquire_time));
	pkt->data.info.az = az_interp; 
	pkt->data.info.el = el_interp; 
	pkt->data.info.scan_type = PMACDPRAMData[FirstPMACDPRAMInterpidx].scan_type;
	pkt->data.info.scan_num = PMACDPRAMData[FirstPMACDPRAMInterpidx].sweep; // scan_num
	pkt->data.info.vol_num = PMACDPRAMData[FirstPMACDPRAMInterpidx].volume;
	pkt->data.info.transition = PMACDPRAMData[FirstPMACDPRAMInterpidx].transition;
	daI = az_interp - last_ai; deI = el_interp - last_ei;
//printf("%d\n", pkt->data.info.channel);
//azI, elI interpolated angles put in packet: 
//	fprintf(db_fp, "REMSec = %I64d: azI = %4.3f daI = %4.3f elI = %4.3f deI = %4.3f\n", RadarEpochMillisec, az_interp, daI, el_interp, deI); 
//printf("REMSec = %I64d\ntN+1   = %I64d \ntN     = %I64d idxN+1 = %d idxN = %d idxCur = %d aN+1 = %4.3f aN = %4.3f\n\n", RadarEpochMillisec, PMACDPRAMData[SecondPMACDPRAMInterpidx].acquire_time, PMACDPRAMData[FirstPMACDPRAMInterpidx].acquire_time, SecondPMACDPRAMInterpidx, FirstPMACDPRAMInterpidx, PMACDPRAMDataidx, PMACDPRAMData[SecondPMACDPRAMInterpidx].az_avg, PMACDPRAMData[FirstPMACDPRAMInterpidx].az_avg); 
	last_ai = az_interp; last_ei = el_interp; 
	_ftime( &PMACtimebuffer ); millisec = PMACtimebuffer.millitm; 
//printf("PMACDPRAMData: sweep=%d volume=%d\n", PMACDPRAMData[FirstPMACDPRAMInterpidx].sweep, PMACDPRAMData[FirstPMACDPRAMInterpidx].volume); 
	return(TRUE);
}

/* perform conversion and correction to short I's and Q's */
char   PHASE;
void convert(PACKET *pkt)
   {
   int		i,gates4;
   short		*sptr;
   float		*fptr;
   
   sptr = (short *)pkt->data.data;
   // create test sinusoid w/constantly rotating PHASE. 
#if 0
   PHASE++;
   sptr[0] = 32768.0 * 0.5 * sin(2 * M_PI * PHASE / 256.0);
   sptr[1] = 32768.0 * 0.5 * cos(2 * M_PI * PHASE / 256.0);
   sptr[2] = 32768.0 * 0.25 * sin(2 * M_PI * PHASE / 256.0);
   sptr[3] = 32768.0 * 0.25 * cos(2 * M_PI * PHASE / 256.0);
   sptr[4] = 32768.0 * 0.5 * sin(2 * M_PI * PHASE / 256.0);
   sptr[5] = 32768.0 * 0.5 * cos(2 * M_PI * PHASE / 256.0);
   sptr[6] = 32768.0 * 0.25 * sin(2 * M_PI * PHASE / 256.0);
   sptr[7] = 32768.0 * 0.25 * cos(2 * M_PI * PHASE / 256.0);
#endif
#if 0
   sptr[0] = 0x7001; // easily visible test data marking begin of datablock
   sptr[1] = 0x6002;
   sptr[2] = 0x5003;
   sptr[3] = 0x4004;
   sptr[4] = 0x5003;
   sptr[5] = 0x4004;
#endif
//   gates4 = pkt->data.info.numgates * 4;

//   sptr = (short *)pkt->data.data;
//   fptr = (float *)pkt->data.data;
//   for(i=0; i<gates4; i++)
//      *sptr++ = *fptr++ * 32768.0;

   }


FIFO *open_piraq_fifo(char *name, int headersize, int recordsize, int recordnum)
   {
   int		loop=0;
   FIFO	*fifo;
   
   printf("Opening FIFO %s......",name); 
//   fifo = fifo_create(name,headersize,recordsize,recordnum,0);
   fifo = fifo_create(name,headersize,recordsize,recordnum); 
   if(!fifo)	{printf("\nCannot open %s FIFO buffer\n",name); exit(-1);}
   printf("   done.\n");
   while(fifo->magic != MAGIC) /* if created by another task, wait for initialization */
      {
      loop = 1;
      printf("Waiting for %s FIFO to be initialized by parent task\r",name); 
      }
   if(loop)   printf("\n");		/* finish off the print statement from loop */

   return(fifo);
   }
  
#if 0 // not used 
void shortdatabit(unsigned short which,int size)
   {
   int  i,temp,stuckhi,stucklo,unreliable;
   
   stuckhi = size - 1;
   stucklo = size - 1;
   unreliable = 0x0000;

   for(i=0; i<size; i++)
      {
      //*addr = i;
		if(which == 0) 
		{
			piraq->GetControl()->SetValue_StatusRegister0(i);
			temp = piraq->GetControl()->GetValue_StatusRegister0() & (size-1);
		}
		else
		{
			piraq->GetControl()->SetValue_StatusRegister1(i);
			temp = piraq->GetControl()->GetValue_StatusRegister1() & (size-1);
		}
		//temp = *(volatile unsigned short *)addr & (size - 1);
		stuckhi &=  temp;
		stucklo &= ~temp;
		unreliable |= i ^ temp;
      }

   /* ignore certain bits */
//   stuckhi    &= ~0x0E04;
//   stucklo    &= ~0x0E04;
//   unreliable &= ~0x0E04;

   if(stuckhi)
      {printf("Bits stuck high: "); printbits(stuckhi);}

   if(stucklo)
      {printf("Bits stuck low: "); printbits(stucklo);}

   if(unreliable)
      {printf("Unreliable Bits: "); printbits(unreliable);}

   if(!stuckhi && !stucklo && !unreliable)
      printf("Passes\n");
   }
#endif

void printbits(int num)
   {
   int  i;

   for(i=0; i<32; i++)
      if((num >> i) & 1)
         printf("%d ",i);
   printf("\n");
   }

void shortmembit(unsigned short addr[],int size)
  /* size = number of shorts */
   {
   int  i,temp,stuckhi,stucklo,unreliable;
   
   stuckhi = size - 1;
   stucklo = size - 1;
   unreliable = 0x0000;

   for(i=0; i<size; i++)
      addr[i] = size - i;

   for(i=0; i<size; i++)
      {
      temp = addr[i] & 0xFFFF;
      stuckhi &=  temp;
      stucklo &= ~temp;
      unreliable |= (size - i) ^ temp;
      }

   if(stuckhi)
      {printf("Bits stuck high: "); printbits(stuckhi);}

   if(stucklo)
      {printf("Bits stuck low: "); printbits(stucklo);}

   if(unreliable)
      {printf("Unreliable Bits: "); printbits(unreliable);}

   if(!stuckhi && !stucklo && !unreliable)
      printf("Passes\n");
   }
