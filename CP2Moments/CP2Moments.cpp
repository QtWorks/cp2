#include "CP2Moments.h"
#include "CP2Net.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <qlabel.h>
#include <qpushbutton.h>

CP2Moments::CP2Moments():
CP2MomentsBase(),
_pPulseSocket(0),    
_pPulseSocketNotifier(0),
_pPulseSocketBuf(0),	
_Sparams(Params::DUAL_FAST_ALT,100),
_Xparams(Params::DUAL_CP2_XBAND,100),
_collator(5000),
_statsUpdateInterval(5),
_processSband(true),
_processXband(true)
{
	// assign the incoming and outgoing port numbers.
	_timeSeriesPort	= 3100;
	_productsPort   = 3200;

	// initialize the statictics and error monitoring.
	for (int i = 0; i < 3; i++) {
		_pulseCount[i]	= 0;
		_prevPulseCount[i] = 0;
		_errorCount[i] = 0;
		_eof[i] = false;
		_lastPulseNum[i] = 0;
	}
	_sBeamCount = 0;
	_xBeamCount = 0;

	// create the moments processing threads
	_pSmomentThread = new MomentThread(Params::DUAL_FAST_ALT, 100);
	_pXmomentThread = new MomentThread(Params::DUAL_CP2_XBAND, 100);

	// start the moments processing threads. They will wait
	// patiently until their processPulse() functions are
	// called with pulses to be processed.
	_pSmomentThread->start();
	_pXmomentThread->start();

	// intialize the data reception socket.
	// set up the ocket notifier and connect it
	// to the data reception slot
	initializeSockets();	

	// update the statistics
	timerEvent(0);

	// set the run state
	startStopSlot(false);

	// start the statistics timer
	startTimer(_statsUpdateInterval*1000);

}
/////////////////////////////////////////////////////////////////////
CP2Moments::~CP2Moments()
{
}
/////////////////////////////////////////////////////////////////////
void
CP2Moments::startStopSlot(bool v)
{
	// When the button is pushed in, we are stopped
	_run = !v;
	// set the button text to the opposite of the
	// current state.
	if (!_run) {
		_startStopButton->setText("Start");
		_statusText->setText("Stopped");
	} else {
		_startStopButton->setText("Stop");
		_statusText->setText("Running");
	}
}
/////////////////////////////////////////////////////////////////////
void
CP2Moments::timerEvent(QTimerEvent*) 
{
	int rate[3];
	for (int i = 0; i < 3; i++) {
		rate[i] = (_pulseCount[i]-_prevPulseCount[i])/(float)_statsUpdateInterval;
		_prevPulseCount[i] = _pulseCount[i];
	}

	int collatorErrors = _collator.discards();

	_piraq1Errors->setNum(_errorCount[0]);
	_piraq2Errors->setNum(_errorCount[1]);
	_piraq3Errors->setNum(_errorCount[2]);

	_piraq1rate->setNum(rate[0]);
	_piraq2rate->setNum(rate[1]);
	_piraq3rate->setNum(rate[2]);

	_sBeams->setNum(_sBeamCount);
	_xBeams->setNum(_xBeamCount);

	_collatorErrors->setNum(collatorErrors);

}
//////////////////////////////////////////////////////////////////////
void
CP2Moments::initializeSockets()	
{
	// allocate the buffer that will recieve the incoming pulse data
	_pPulseSocketBuf = new char[1000000];

	// create the sockets
	// this initializes the socket system, otherwise gethostname()
	// will fail (below).
	_pPulseSocket = new QSocketDevice(QSocketDevice::Datagram);
	_pProductSocket = new QSocketDevice(QSocketDevice::Datagram);

	_pPulseSocket->setAddressReusable(true);
	_pProductSocket->setAddressReusable(true);

	// find out our IP address and name
	char nameBuf[1000];
	if (gethostname(nameBuf, sizeof(nameBuf))) {
		qWarning("gethostname failed");
		exit(1);
	}
	//strcpy(nameBuf, "127.0.0.1");
	struct hostent* pHostEnt = gethostbyname(nameBuf);
	if (!pHostEnt) {
		qWarning("gethostbyname failed");
		exit(1);
	}

	// display some of the IP information
	std::string myIPname = nameBuf;
	std::string myIPaddress = inet_ntoa(*(struct in_addr*)pHostEnt->h_addr_list[0]);
	std::cout << "ip name: " << myIPname.c_str() << ", id address  " << myIPaddress.c_str() << std::endl;
	//	m_pTextIPname->setText(myIPname.c_str());
	//	m_pTextIPaddress->setNum(+_timeSeriesPort);	// diagnostic print

	QHostAddress qHost;
	qHost.setAddress(myIPaddress.c_str());

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	// Set up the pulse incoming socket.
	// bind to the pulse socket
	if (!_pPulseSocket->bind(qHost, _timeSeriesPort)) {
		qWarning("Unable to bind to %s:%d", qHost.toString().ascii(), _timeSeriesPort);
		exit(1); 
	}

	// set the system socket buffer size - necessary to avoid dropped pulses.
	_pPulseSocket->setReceiveBufferSize(CP2MOMENTS_PULSE_RCVBUF);

	// set up the socket notifier for pulse datagrams
	_pPulseSocketNotifier = new QSocketNotifier(_pPulseSocket->socket(), QSocketNotifier::Read);
	// and connect to pulse slot
	connect(_pPulseSocketNotifier, SIGNAL(activated(int)), this, SLOT(newPulseDataSlot(int)));

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	// Set up product outgoing socket.

	// set the system socket buffer size - to try to
	// avoid blocking on data sends.
	_pProductSocket->setSendBufferSize(CP2MOMENTS_PROD_SNDBUF);

	std::vector<char> n;
	for (unsigned int i=0; i < myIPaddress.size(); i++)
		n.push_back(myIPaddress[i]);
	n.push_back(0);
	Q_UINT32 a = GetBroadcastAddress(&n[0]);
	_outHostAddr.setAddress(a);
	QString s = _outHostAddr.toString();
	_outHostIP = s;

	_outHostAddr.setAddress("192.168.3.255");

	_soMaxMsgSize = 64000;

}
//////////////////////////////////////////////////////////////////////
void 
CP2Moments::newPulseDataSlot(int)
{
	int	readBufLen = _pPulseSocket->readBlock(
		(char *)_pPulseSocketBuf, 
		sizeof(short)*1000000);

	if (readBufLen > 0) {
		// put this datagram into a packet
		bool packetBad = _pulsePacket.setPulseData(
			readBufLen, 
			_pPulseSocketBuf);

		// Extract the pulses and process them.
		// Observe paranoia for validating packets and pulses.
		// From here on out, we are divorced from the
		// data transport.
		if (!packetBad) {
			for (int i = 0; i < _pulsePacket.numPulses(); i++) {
				CP2Pulse* pPulse = _pulsePacket.getPulse(i);
				if (pPulse) {
					int chan = pPulse->header.channel;
					if (chan >= 0 && chan < 3) {


						// do all of the heavy lifting for this pulse,
						// but only if processing is enabled.
						if (_run)
							processPulse(pPulse);

						// look for pulse number errors
						int chan = pPulse->header.channel;

						// check for consecutive pulse numbers
						if (_lastPulseNum[chan]) {
							if (_lastPulseNum[chan]+1 != pPulse->header.pulse_num) {
								_errorCount[chan]++;
							}
						}
						_lastPulseNum[chan] = pPulse->header.pulse_num;

						// count the pulses
						_pulseCount[chan]++;

						// look for eofs.
						if (pPulse->header.status & PIRAQ_FIFO_EOF) {
							switch (chan) 
							{
							case 0:
								if (!_eof[0]) {
									_eof[0] = true;
									//_chan0led->setBackgroundColor(QColor("red"));
								}
								break;
							case 1:
								if (!_eof[1]) {
									_eof[1] = true;
									//_chan1led->setBackgroundColor(QColor("red"));
								}
								break;
							case 2:
								if (!_eof[2]) {
									_eof[2] = true;
									//_chan2led->setBackgroundColor(QColor("red"));
								}
								break;
							}
						}
					} 
				} 
			}
		} 
	} else {
		// read error. What should we do here?
	}
}
//////////////////////////////////////////////////////////////////////
void
CP2Moments::processPulse(CP2Pulse* pPulse) 
{

	// beam will point to computed moments when they are ready,
	// or null if not ready
	Beam* pBeam = 0;

	int chan = pPulse->header.channel;
	if (chan == 0) 
	{	
		// S band pulses: are successive coplaner H and V pulses
		// this horizontal switch is a hack for now; we really
		// need to find the h/v flag in the pulse header.
		_azSband += 1.0/_Sparams.moments_params.n_samples;
		if (_azSband > 360.0)
			_azSband = 0.0;

		pPulse->header.antAz = _azSband;
		CP2FullPulse* pFullPulse = new CP2FullPulse(pPulse);
		if (_processSband) {
			_pSmomentThread->processPulse(pFullPulse, 0);
		} else {
			delete pFullPulse;
		}

		// ask for a completed beam. The return value will be
		// 0 if nothing is ready yet.
		pBeam = _pSmomentThread->getNewBeam();
		if (pBeam) {
			sDatagram(pBeam);
			delete pBeam;
			_sBeamCount++;
		}
	} else {
		// X band will have H coplanar pulse on channel 1
		// and V cross planar pulses on channel 2. We need
		// to buffer up the data from the pulses and send it to 
		// the collator to match beam numbers, since matching
		// pulses (i.e. identical pulse numbers) are required
		// for the moments calculations.

		// create a CP2FullPuse, which is a class that will
		// hold the IQ data.
		CP2FullPulse* pFullPulse = new CP2FullPulse(pPulse);
		// send the pulse to the collator. The collator finds matching 
		// pulses. If orphan pulses are detected, they are deleted
		// by the collator. Otherwise, matching pulses returned from
		// the collator can be deleted here.
		_collator.addPulse(pFullPulse, chan-1);

		// now see if we have some matching pulses
		CP2FullPulse* pHPulse;
		CP2FullPulse* pVPulse;
		if (_collator.gotMatch(&pHPulse, &pVPulse)) {
			_azXband += 1.0/_Sparams.moments_params.n_samples;
			if (_azXband > 360.0)
				_azXband = 0.0;
			pHPulse->header()->antAz = _azXband;
			pVPulse->header()->antAz = _azXband;

			// a matching pair was found. Send them to the X band
			// moments compute engine.
			if (_processXband) {
				_pXmomentThread->processPulse(pHPulse, pVPulse);
			} else {

				// finished with these pulses, so delete them.
				delete pHPulse;
				delete pVPulse;
			}
			// ask for a completed beam. The return value will be
			// 0 if nothing is ready yet.
			pBeam = _pXmomentThread->getNewBeam();
		} else {
			pBeam = 0;
		}
		if (pBeam) {
			// we have X products
			xDatagram(pBeam);
			delete pBeam;
			_xBeamCount++;
		}
	}
}
////////////////////////////////////////////////////////////////////
void 
CP2Moments::sendProduct(CP2ProductHeader& header, 
						std::vector<double>& data,
						CP2Packet& packet,
						bool forceSend)
{
	int incr = data.size()*sizeof(data[0]) + sizeof(header);
	// if this packet will get too large by adding new data, 
	// go ahead and send it
	if (packet.packetSize()+incr > _soMaxMsgSize) {
		int n = _pProductSocket->writeBlock(
			(const char*)packet.packetData(),
			packet.packetSize(),
			_outHostAddr,
			_productsPort);
		int s = _xProductPacket.packetSize();
		int err = WSAGetLastError();

		packet.clear();
	}

	packet.addProduct(header, data.size(), &data[0]);
	if (forceSend) {
		int n = _pProductSocket->writeBlock(
			(const char*)packet.packetData(),
			packet.packetSize(),
			_outHostAddr,
			_productsPort);
		int s = _xProductPacket.packetSize();
		int err = WSAGetLastError();

		packet.clear();
	}
}
////////////////////////////////////////////////////////////////////
void 
CP2Moments::sDatagram(Beam* pBeam)
{
	int gates = pBeam->getNGatesOut();

	_sProductPacket.clear();
	_sProductData.resize(gates);

	CP2ProductHeader header;
	header.beamNum = pBeam->getSeqNum();
	header.gates   = gates;
	header.antAz   = pBeam->getAz();
	header.antEl   = pBeam->getEl();

	const Fields* fields = pBeam->getFields();

	header.prodType = PROD_S_DBMHC;	///< S-band dBm horizontal co-planar
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].dbmhc;  }
	sendProduct(header, _sProductData, _sProductPacket);

	header.prodType = PROD_S_DBMVC;	///< S-band dBm vertical co-planar
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].dbmvc;  }
	sendProduct(header, _sProductData, _sProductPacket);

	header.prodType = PROD_S_DBZHC;	///< S-band dBz horizontal co-planar
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].dbzhc;  }
	sendProduct(header, _sProductData, _sProductPacket);

	header.prodType = PROD_S_DBZVC;	///< S-band dBz vertical co-planar
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].dbzvc;  }
	sendProduct(header, _sProductData, _sProductPacket);

	header.prodType = PROD_S_RHOHV;	///< S-band rhohv
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].rhohv;  }
	sendProduct(header, _sProductData, _sProductPacket);

	header.prodType = PROD_S_PHIDP;	///< S-band phidp
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].phidp;  }
	sendProduct(header, _sProductData, _sProductPacket);

	header.prodType = PROD_S_ZDR;	///< S-band zdr
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].zdr;  }
	sendProduct(header, _sProductData, _sProductPacket);

	header.prodType = PROD_S_WIDTH;	///< S-band spectral width
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].width;  }
	sendProduct(header, _sProductData, _sProductPacket);

	header.prodType = PROD_S_VEL;		///< S-band velocity
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].vel;    }
	sendProduct(header, _sProductData, _sProductPacket);

	header.prodType = PROD_S_SNR;		///< S-band SNR
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].snr;    }
	sendProduct(header, _sProductData, _sProductPacket, true);

}
/////////////////////////////////////////////////////////////////////
void 
CP2Moments::xDatagram(Beam* pBeam)
{
	int gates = pBeam->getNGatesOut();

	_xProductPacket.clear();
	_xProductData.resize(gates);

	CP2ProductHeader header;
	header.beamNum = pBeam->getSeqNum();
	header.gates   = gates;
	header.antAz   = pBeam->getAz();
	header.antEl   = pBeam->getEl();

	const Fields* fields = pBeam->getFields();

	header.prodType = PROD_X_DBMHC;
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].dbmhc;}
	sendProduct(header, _xProductData, _xProductPacket);

	header.prodType =  PROD_X_DBZHC;	///< X-band dBz horizontal co-planar
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].dbzhc;  } 
	sendProduct(header, _xProductData, _xProductPacket);

	header.prodType = PROD_X_SNR;		///< X-band SNR
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].snr;    }
	sendProduct(header, _xProductData, _xProductPacket);

	header.prodType = PROD_X_LDR;		///< X-band LDR
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].ldrh;   }
	sendProduct(header, _xProductData, _xProductPacket, true);

}
/////////////////////////////////////////////////////////////////////
// This function calculates a suitable broadcast address to use with
// the supplied IP number.
// It will do it by locating the subnet mask for that IP number. Then
// it will calculate the broadcast address as:
// broadcast = ip | ~(subnet)
// The reason for this function is that the request for broadcast address
// in WSAIoctl() often returns 255.255.255.255 even if a more narrow broadcast
// address should be used.
//
unsigned long 
CP2Moments::GetBroadcastAddress(char* IPname)
{
	DWORD dwIp = inet_addr(IPname);
	// Create a socket that is just used to get the IP info
	SOCKET s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(s == INVALID_SOCKET) {
		return INADDR_BROADCAST;
	}

	// Gather the IP Info
	INTERFACE_INFO info[MAX_NBR_OF_ETHERNET_CARDS];
	DWORD dwBytesRead;
	if(WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0,
		(void*)info, sizeof(info), &dwBytesRead, NULL, NULL) != 0) {
			closesocket(s);
			return INADDR_BROADCAST;
		}

		DWORD dwBroadcast = INADDR_BROADCAST;

		// Search all ip infos for the requested ip number
		for(unsigned int i = 0; i < (dwBytesRead / sizeof(INTERFACE_INFO)); i++) {
			// Notice: the INTERFACE_INFO contains a broadcast address
			// but that can't be used since it is always 255.255.255.255
			if(info[i].iiAddress.AddressIn.sin_addr.s_addr == dwIp) {
				DWORD dwSubnet = info[i].iiNetmask.AddressIn.sin_addr.s_addr;
				dwBroadcast = dwIp | ~(dwSubnet);
				break;
			}
		}

		closesocket(s);

		return dwBroadcast; 
}

