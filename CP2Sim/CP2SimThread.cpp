#include "CP2SimThread.h"
#include <QMessageBox>

#ifdef WIN32
#include <Windows.h>  // just to get Sleep()
#else
#define Sleep(x) usleep(x*1000)
#endif


///////////////////////////////////////////////////////////////////////////////////
CP2SimThread::CP2SimThread():
_config("NCAR", "CP2Sim"),
_run(false),
_gates(950),
_pulsesPerDatagram(8),
_simAngles(SimAngles::PPI, 100, 1.0, 79.0, 1.0, 1.0, 18.0, 0.88, 243),
_pulseCount(0),
_quitThread(false)
{	
	// intialize the data transmission socket.
	initializeSocket();	

	// initialize the angle simulator
	_simAngles = createSimAngles();

	// allocate space to create data
	_sData.resize(2*_gates);
	_xhData.resize(2*_gates);
	_xvData.resize(2*_gates);
}

///////////////////////////////////////////////////////////////////////////////////
CP2SimThread::~CP2SimThread(void)
{
	if (_pPulseSocket)
		delete _pPulseSocket;
}

///////////////////////////////////////////////////////////////////////////////////
void
CP2SimThread::run()
{
	while (!_quitThread) {
	  Sleep((int)(1*_pulsesPerDatagram*0.8));
		if (_run) {
			nextPulses();
		}
	}

}

//////////////////////////////////////////////////////////////////////
void
CP2SimThread::nextPulses()
{
	// if we don't have a working socket, ignore the request
	if (!_pPulseSocket)
		return;
	// set up a header
	CP2PulseHeader header;
	header.channel = 0;

	_sPacket.clear();
	_xhPacket.clear();
	_xvPacket.clear();

	// add all beams to the outgoing packet and to the pulse queue
	for (int i = 0; i < _pulsesPerDatagram; i++) {
		header.scanType  = 1;
		header.antSize   = 0;
		header.pulse_num = _pulseCount;
		header.gates     = 950;
		header.status    = 0;
		header.prt       = 1.0E3; //_prt;
		header.xmit_pw   = 1.0e-6; //xmit_pulsewidth;
		header.horiz     = true;

		// set the antenna information. 
		_simAngles.nextAngle(header.az, header.el, header.antTrans, header.sweepNum, header.volNum);

		// Save the current antenna information
		//_az     = header.az;
		//_el     = header.el;
		//_volume = header.volNum;
		//_sweep  = header.sweepNum;

		// add pulse to the outgoing packets
		header.channel = 0;
		_sPacket.addPulse(&header,  header.gates*2, &_sData[0]);
		header.channel = 1;
		_xhPacket.addPulse(&header, header.gates*2, &_xhData[0]);
		header.channel = 2;
		_xvPacket.addPulse(&header, header.gates*2, &_xvData[0]);

		_pulseCount++;

	}

	int bytesSent;
	bytesSent = sendData(_sPacket.packetSize(),   _sPacket.packetData());
	bytesSent = sendData(_xhPacket.packetSize(), _xhPacket.packetData());
	bytesSent = sendData(_xvPacket.packetSize(), _xvPacket.packetData());
}
///////////////////////////////////////////////////////////////////////////
int 
CP2SimThread::sendData(int size,  void* data)
{
	int bytesSent;

	/// @todo Instead of just looping here, we should look into
	/// the logic of coming back later and doing the resend. I tried
	/// putting a Sleep(1) before each resend; for some strange reason
	/// this caused two of the three piraqs to stop sending data.
	do {
		bytesSent = _pPulseSocket->writeDatagram((const char*)data, size);
		if (bytesSent != size) {
			_resendCount++;
		}
	} while (bytesSent != size);

	return bytesSent;
}

//////////////////////////////////////////////////////////////////////
void
CP2SimThread::setRunState(bool runState) 
{
	_run = runState;
}

//////////////////////////////////////////////////////////////////////
void
CP2SimThread::initializeSocket()	
{
	// assign the incoming and outgoing port numbers.
	_pulsePort	    = _config.getInt("Network/PulsePort", 3100);

	// get the network identifiers
	std::string pulseNetwork   = _config.getString("Network/PulseNetwork", "192.168.1");

	// create the socket
	_pPulseSocket   = new CP2UdpSocket(pulseNetwork, _pulsePort, false, 10000000, 0);

	if (!_pPulseSocket->ok()) {
		std::string errMsg = _pPulseSocket->errorMsg().c_str();
		errMsg += " Pulses will not be created";
		QMessageBox e;
		e.warning(0, "Error", errMsg.c_str(), 
			QMessageBox::Ok, QMessageBox::NoButton);
		delete _pPulseSocket;
		_pPulseSocket = 0;
	}

	// The max datagram message must be smaller than 64K
	_soMaxMsgSize = 64000;

}
///////////////////////////////////////////////////////////////////////////////////
SimAngles
CP2SimThread::createSimAngles()
{
	// if PPImode is true, do PPI else RHI
	bool ppiMode = _config.getBool("SimulatedAngles/ppiMode", true);
	SimAngles::SIMANGLESMODE mode = SimAngles::PPI;
	if (!ppiMode)
		mode = SimAngles::RHI;

	int pulsesPerBeam = _config.getInt("SimulatedAngles/pulsesPerBeam", 100);
	double beamWidth = _config.getDouble("SimulatedAngles/beamWidth", 1.0);
	double rhiAzAngle = _config.getDouble("SimulatedAngles/rhiAzAngle", 37.5);
	double ppiElIncrement = _config.getDouble("SimulatedAngles/ppiElIncrement", 2.5);
	double elMinAngle = _config.getDouble("SimulatedAngles/elMinAngle", 3.0);
	double elMaxAngle = _config.getDouble("SimulatedAngles/elMaxAngle", 42.0);
	double sweepIncrement = _config.getDouble("SimulatedAngles/sweepIncrement", 3.0);
	int numPulsesPerTransition = _config.getInt("SimulatedAngles/pulsesPerTransition", 31);

	SimAngles simAngles(
		mode,
		pulsesPerBeam,
		beamWidth,
		rhiAzAngle,
		ppiElIncrement,
		elMinAngle,		    
		elMaxAngle,
		sweepIncrement,
		numPulsesPerTransition );

	return simAngles;

}
///////////////////////////////////////////////////////////////////////////////////
int
CP2SimThread::getPulseCount()
{
	return _pulseCount;
}
///////////////////////////////////////////////////////////////////////////////////
void
CP2SimThread::end() {
  _quitThread = true;
}
