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

#include <qmessagebox.h>

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
_piraq0(0),
_piraq1(0),
_piraq2(0),
_pulses1(0),
_pulses2(0),
_pulses3(0),
_stop(false),
_status(STARTUP),
_config("NCAR", "CP2Exec"),
_pPulseSocket(0)
{
}

/////////////////////////////////////////////////////////////////////
CP2ExecThread::~CP2ExecThread()
{
	delete _pPulseSocket;
}

/////////////////////////////////////////////////////////////////////////////
unsigned int
CP2ExecThread::findPMACdpram() 
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
void
CP2ExecThread::run()
{
	_status = PIRAQINIT;

	float prt;
	float xmit_pulsewidth;

	CONFIG *config1, *config2, *config3;
	char fname1[100]; 
	char fname2[100]; 
	char fname3[100]; // configuration filenames

	char c;
	int piraqs = 0;   // board count -- default to single board operation 
	FILE * dspEXEC; 

	long long pulsenum;
	unsigned int PMACphysAddr;

	int	PIRAQadjustAmplitude = 0; 
	int	PIRAQadjustFrequency = 1; 

	/// @todo Conslidate PCI Timer control into a class.
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
	_pulsesPerPciXfer = 65536 / blocksize; 
	if	(_pulsesPerPciXfer % 2)	//	computed odd #hits
		_pulsesPerPciXfer--;	//	make it even

	// find the PMAC card
	PMACphysAddr = findPMACdpram();
	if (PMACphysAddr == 0) {
		printf("unable to locate PMAC dpram, piraqs will be told to ignore PMAC\n");
	}
	printf("PMAC DPRAM base addr is 0x%08x\n", PMACphysAddr);

	// get the network designation for the network that
	// pulses will be broadcast to. This can be a complete
	// interface address, such as 192.168.1.3, or 127.0.0.1,
	// or it can be just the network address such as 192.168.1
	std::string pulseNetwork = _config.getString("Network/PulseNetwork", "192.168.1");
	// Get the output port
	_pulsePort = _config.getInt("Network/PulsePort", 3100); 

	_pPulseSocket = new CP2UdpSocket(pulseNetwork, _pulsePort, true, 10000000, 0);
	if (!_pPulseSocket->ok()) {
		std::string errMsg = "Error opening the network port ";
		errMsg += pulseNetwork;
		errMsg += ": ";
		errMsg += _pPulseSocket->errorMsg().c_str();
		QMessageBox e;
		e.warning(0, "Network Error", errMsg.c_str(), 
			QMessageBox::Ok, QMessageBox::NoButton);
	}

	///////////////////////////////////////////////////////////////////////////
	//
	//    Create piraqs. They all have to be created, so that all boards
	//    are found in succesion, even if we will not be collecting data 
	//    from all of them.

	char* dname = new char[_dspObjFile.size()+1];
	strcpy(dname, _dspObjFile.c_str());

	_piraq0 = new CP2PIRAQ(_pPulseSocket, fname1, dname, 
		_pulsesPerPciXfer, PMACphysAddr, 0, SHV);
	_piraq1 = new CP2PIRAQ(_pPulseSocket, fname2, dname, 
		_pulsesPerPciXfer, PMACphysAddr, 1, XH);
	_piraq2 = new CP2PIRAQ(_pPulseSocket, fname3, dname, 
		_pulsesPerPciXfer, PMACphysAddr, 2, XV);

	prt = _piraq2->prt();
	xmit_pulsewidth = _piraq2->xmit_pulsewidth();
	delete [] dname;

	///////////////////////////////////////////////////////////////////////////
	//
	//     Calculate starting beam and pulse numbers. 
	//     These are passed on to the piraqs, so that they all
	//     start with the same beam and pulse number.

	unsigned int pri; 
	pri = (unsigned int)((((float)COUNTFREQ)/(float)(1/prt)) + 0.5); 
	time_t now = time(&now);
	pulsenum = ((((long long)(now+2)) * (long long)COUNTFREQ) / pri) + 1; 
	printf("pulsenum = %I64d\n", pulsenum); 

	///////////////////////////////////////////////////////////////////////////
	//
	//      start the piraqs, waiting for each to indicate they are ready

	if (_piraq0->start(pulsenum)) {
		std::cerr << "unable to start piraq1\n";
		exit();
	} 
	if (_piraq1->start(pulsenum)) {
		std::cerr << "unable to start piraq2\n";
		exit();
	} 
	if (_piraq2->start(pulsenum)) {
		std::cerr << "unable to start piraq3\n";
		exit();
	} 

	std::cout 
		<< "\nAll piraqs have been started. "
		<< _pulsesPerPciXfer 
		<< " pulses will be \ntransmitted for each PCI bus transfer\n\n";

	///////////////////////////////////////////////////////////////////////////
	//
	// start timer board immediately

	PINFOHEADER info;
	info = _piraq2->info();
	cp2timer_config(config1, &ext_timer, &info, prt, xmit_pulsewidth);
	cp2timer_set(&ext_timer);		// put the timer structure into the timer DPRAM 
	cp2timer_reset(&ext_timer);	// tell the timer to initialize with the values from DPRAM 
	cp2timer_start(&ext_timer);	// start timer 

	_status = RUNNING;

	while(1) { 
		_pulses1 += _piraq0->poll();
		_pulses2 += _piraq1->poll();
		_pulses3 += _piraq2->poll();

		if (_stop)
			break;
	}

	// remove for lab testing: keep transmitter pulses 
	// active w/o go.exe running. 12-9-04
	cp2timer_stop(&ext_timer); 

	if (_piraq0)
		delete _piraq0; 
	if (_piraq1)
		delete _piraq1; 
	if (_piraq2)
		delete _piraq2;

	_piraq0 = 0;
	_piraq1 = 0;
	_piraq2 = 0;

	_stop = false;

	// The thread will finish execution when it returns 
	// from run()  (right here!). The owner can wait
	// for this by calling QThread::wait()

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
	if (!_piraq0 || !_piraq1 || !_piraq2) {
		errors1 = 0.0;
		errors2 = 0.0;
		errors3 = 0.0;
		return;
	}

	errors1 = _piraq0->pnErrors();
	errors2 = _piraq1->pnErrors();
	errors3 = _piraq2->pnErrors();

}/////////////////////////////////////////////////////////////////////
void 
CP2ExecThread::antennaInfo(double* az, double* el, 
						   unsigned int* sweep, unsigned int* volume)
{
	if (!_piraq0 || !_piraq1 || !_piraq2) {
		for (int i = 0; i < 3; i++) {
			az[i] = 0;
			el[i] = 0;
			sweep[i] = 0;
			volume[i] = 0;
		}
		return;
	}

	_piraq0->antennaInfo(az[0], el[0], sweep[0], volume[0]);
	_piraq1->antennaInfo(az[1], el[1], sweep[1], volume[1]);
	_piraq2->antennaInfo(az[2], el[2], sweep[2], volume[2]);
}
/////////////////////////////////////////////////////////////////////
void 
CP2ExecThread::rates(double& rate1, double& rate2, double& rate3)
{
	if (!_piraq0 || !_piraq1 || !_piraq2) {
		rate1 = 0.0;
		rate2 = 0.0;
		rate3 = 0.0;
		return;
	}

	rate1 = _piraq0->sampleRate();
	rate2 = _piraq1->sampleRate();
	rate3 = _piraq2->sampleRate();
}
/////////////////////////////////////////////////////////////////////
void
CP2ExecThread::pulses(int& pulses1, int& pulses2, int& pulses3) {
	pulses1 = _pulses1;
	pulses2 = _pulses2;
	pulses3 = _pulses3;
}

/////////////////////////////////////////////////////////////////////
void
CP2ExecThread::eof(bool eofflags[3])
{
	if (!_piraq0 || !_piraq1 || !_piraq2) 
	{
		eofflags[0] = false;
		eofflags[1] = false;
		eofflags[2] = false; 
		return;
	}
	eofflags[0] = _piraq0->eof();
	eofflags[1] = _piraq1->eof();
	eofflags[2] = _piraq2->eof(); 

}
/////////////////////////////////////////////////////////////////////
CP2ExecThread::STATUS
CP2ExecThread::status()
{
	return _status;
}
/////////////////////////////////////////////////////////////////////
