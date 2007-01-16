#include <winsock2.h>
#include "CP2ExecThread.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include <windows.h>
#include <mmsystem.h>
#include <sys/timeb.h>
#include <time.h>
#include <iostream>

// local includes
#include "CP2exec.h" 
#include "CP2PIRAQ.h"

// from CP2Lib
#include "timerlib.h"
#include "config.h"
#include "pci_w32.h"

// from CP2Piraq
#include "piraqComm.h"

// from PiraqIII_RevD_Driver
#include "piraq.h"
#include "plx.h"
#include "control.h"
#include "HPIB.h"

CP2ExecThread::CP2ExecThread(std::string dspObjFile, std::string configFile):
_dspObjFile(dspObjFile),
_configFile(configFile),
_piraq1(0),
_piraq2(0),
_piraq3(0),
_pulses1(0),
_pulses2(0),
_pulses3(0),
_stop(false),
_outport(3100)
{

}

/////////////////////////////////////////////////////////////////////
CP2ExecThread::~CP2ExecThread()
{

}

/////////////////////////////////////////////////////////////////////////////
unsigned int
findPMACdpram() 
{
	PCI_CARD*	pcicard;
	unsigned int reg_base;

	pcicard = find_pci_card(0x1172,1,0);

	if(!pcicard)
		return(0);

	reg_base = pcicard->phys2;

	return reg_base;
}
/////////////////////////////////////////////////////////////////////
/// Initialize the windows network interface. closeNetwork() 
/// must be called the same number of times that this routine is
/// called, because WSAstartup() mantains a reference count. See
/// the windows documentation.
/// @param ipName The network name that datagrams will be sent to
/// @param port the destination port.
/// @param sockAddr A sockAddr structure will be initialized here, so that
///  it can be used later for the sendto() call.
/// @return The socke file descriptor, or -1 if failure.
int 
CP2ExecThread::initNetwork(char* ipName, int port, struct sockaddr_in& sockAddr)
{
	WORD wVersionRequested;
	WSADATA wsaData;

	// startup 
	wVersionRequested = MAKEWORD( 2, 2 );	 
	if (WSAStartup( wVersionRequested, &wsaData )){
		printf ("WARNING - WSAStartup failed - networking may not be avaiable\n");
	}
	int sock = WSASocket(AF_INET,SOCK_DGRAM,IPPROTO_UDP,NULL,0,WSA_FLAG_OVERLAPPED);	//	was dwFlags = 0
	if (sock <0)
		return -1;

	/// initialize sockAddr
	struct hostent* hent = gethostbyname(ipName);
	int inetAddr = *(long*)hent->h_addr;
	sockAddr.sin_port = htons(port);
	sockAddr.sin_addr.s_addr = inetAddr;
	sockAddr.sin_family = AF_INET;

	// ask for a large socket send buffer
	int sockbufsize = 50000000;

	int result = setsockopt (sock,
		SOL_SOCKET,
		SO_SNDBUF,
		(char *) &sockbufsize,
		sizeof sockbufsize);
	if (result) {
		printf("Set send buffer size for socket failed\n");
		exit(); 
	}
	return sock;
}

/////////////////////////////////////////////////////////////////////
void
CP2ExecThread::closeNetwork()
{
	WSACleanup( );
}

/////////////////////////////////////////////////////////////////////
void
CP2ExecThread::run()
{
	CONFIG *config1, *config2, *config3;
	char fname1[100]; char fname2[100]; char fname3[100]; // configuration filenames
	char* destIP = "192.168.3.255";
	//destIP = "127.0.0.1";

	char c;
	int piraqs = 0;   // board count -- default to single board operation 
	FILE * dspEXEC; 

	long long pulsenum;
	unsigned int PMACphysAddr;

	int	PIRAQadjustAmplitude = 0; 
	int	PIRAQadjustFrequency = 1; 
	TIMER ext_timer; // structure defining external timer parameters 

	config1	= new CONFIG; 
	config2	= new CONFIG; 
	config3	= new CONFIG; 

	if ((dspEXEC = fopen(_dspObjFile.c_str(),"r")) == NULL)  
	{ 
		// DSP executable file not found		
		std::cout << "DSP executable file " << _dspObjFile << " not found\n";
		exit(); 
	} 
	fclose(dspEXEC); // existence test passed; use command-line filename

	std::cout << _dspObjFile << " will be loaded into piraqs\n"; 

	strcpy(fname1, _configFile.c_str());
	strcpy(fname2, _configFile.c_str());
	strcpy(fname3, _configFile.c_str());

	printf(" config1 filename %s will be used\n", fname1);
	printf(" config2 filename %s will be used\n", fname2);
	printf(" config3 filename %s will be used\n", fname3);

	// Initialize the network
	_outport = 3100; 
	struct sockaddr_in sockAddr;
	int sock = initNetwork(destIP, _outport, sockAddr);

	// stop timer card
	cp2timer_stop(&ext_timer);  

	// read in fname.dsp, or use configX.dsp if NULL passed. set up all parameters
	readconfig(fname1, config1);    
	readconfig(fname2, config2);    
	readconfig(fname3, config3);   

	/// NOTE- packetsPerPciXfer is computed here from the size of the PPACKET packet, such that
	/// it will be smaller than 64K. This must hold true for the PCI 
	/// bus transfers. 
	int blocksize = sizeof(PINFOHEADER)+
		config1->gatesa * 2 * sizeof(float);
	_packetsPerPciXfer = 65536 / blocksize; 
	if	(_packetsPerPciXfer % 2)	//	computed odd #hits
		_packetsPerPciXfer--;	//	make it even

	// find the PMAC card
	PMACphysAddr = findPMACdpram();
	if (PMACphysAddr == 0) {
		printf("unable to locate PMAC dpram\n");
		exit();
	}
	printf("PMAC DPRAM base addr is 0x%08x\n", PMACphysAddr);

	///////////////////////////////////////////////////////////////////////////
	//
	//    Create piraqs. They all have to be created, so that all boards
	//    are found in succesion, even if we will not be collecting data 
	//    from all of them.

	char* d = new char[_dspObjFile.size()+1];
	strcpy(d, _dspObjFile.c_str());
	_piraq1 = new CP2PIRAQ(sockAddr, sock, destIP, fname1, d, _packetsPerPciXfer, PMACphysAddr, 0, SHV);
	_piraq2 = new CP2PIRAQ(sockAddr, sock, destIP, fname2, d, _packetsPerPciXfer, PMACphysAddr, 1, XH);
	_piraq3 = new CP2PIRAQ(sockAddr, sock, destIP, fname3, d, _packetsPerPciXfer, PMACphysAddr, 2, XV);
	delete [] d;

	///////////////////////////////////////////////////////////////////////////
	//
	//     Calculate starting beam and pulse numbers. 
	//     These are passed on to the piraqs, so that they all
	//     start with the same beam and pulse number.

	float prt;
	prt = _piraq3->prt();
	unsigned int pri; 
	pri = (unsigned int)((((float)COUNTFREQ)/(float)(1/prt)) + 0.5); 
	time_t now = time(&now);
	pulsenum = ((((long long)(now+2)) * (long long)COUNTFREQ) / pri) + 1; 
	printf("pulsenum = %I64d\n", pulsenum); 

	///////////////////////////////////////////////////////////////////////////
	//
	//      start the piraqs, waiting for each to indicate they are ready

	if (_piraq1->start(pulsenum)) {
		std::cerr << "unable to start piraq1\n";
		exit();
	} 
	if (_piraq2->start(pulsenum)) {
		std::cerr << "unable to start piraq2\n";
		exit();
	} 
	if (_piraq3->start(pulsenum)) {
		std::cerr << "unable to start piraq3\n";
		exit();
	} 

	std::cout 
		<< "\nAll piraqs have been started. "
		<< _packetsPerPciXfer 
		<< " pulses will be \ntransmitted for each PCI bus transfer\n\n";

	///////////////////////////////////////////////////////////////////////////
	//
	// start timer board immediately

	PINFOHEADER info;
	info = _piraq3->info();
	cp2timer_config(&ext_timer, &info, config3->prt, config3->xmit_pulsewidth);
	cp2timer_set(&ext_timer);		// put the timer structure into the timer DPRAM 
	cp2timer_reset(&ext_timer);	// tell the timer to initialize with the values from DPRAM 
	cp2timer_start(&ext_timer);	// start timer 

	while(1) { 
		_pulses1 += _piraq1->poll();
		_pulses2 += _piraq2->poll();
		_pulses3 += _piraq3->poll();

		if (_stop)
			break;
	}

	// remove for lab testing: keep transmitter pulses active w/o go.exe running. 12-9-04
	cp2timer_stop(&ext_timer); 

	if (_piraq1)
		delete _piraq1; 
	if (_piraq2)
		delete _piraq2; 
	if (_piraq3)
		delete _piraq3;

	closeNetwork();

}

/////////////////////////////////////////////////////////////////////
void
CP2ExecThread::stop()
{
	_stop = true;
}
/////////////////////////////////////////////////////////////////////
void 
CP2ExecThread::pnErrors(int& errors1, int& errors2, int& errors3)
{
	if (!_piraq1 || !_piraq2 || !_piraq3) {
		errors1 = 0.0;
		errors2 = 0.0;
		errors3 = 0.0;
		return;
	}

	errors1 = _piraq1->pnErrors();
	errors2 = _piraq2->pnErrors();
	errors3 = _piraq3->pnErrors();

}
/////////////////////////////////////////////////////////////////////
void 
CP2ExecThread::rates(double& rate1, double& rate2, double& rate3)
{
	if (!_piraq1 || !_piraq2 || !_piraq3) {
		rate1 = 0.0;
		rate2 = 0.0;
		rate3 = 0.0;
		return;
	}

	rate1 = _piraq1->sampleRate();
	rate2 = _piraq2->sampleRate();
	rate3 = _piraq3->sampleRate();
}
/////////////////////////////////////////////////////////////////////
void
CP2ExecThread::pulses(int& pulses1, int& pulses2, int& pulses3) {
	pulses1 = _pulses1;
	pulses2 = _pulses2;
	pulses3 = _pulses3;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
