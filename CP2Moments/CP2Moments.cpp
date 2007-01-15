#include "CP2Moments.h"
#include "CP2Net.h"
#include <qlabel.h>

CP2Moments::CP2Moments():
CP2MomentsBase(),
_pSocket(0),    
_pSocketNotifier(0),
_pSocketBuf(0),	
_Sparams(Params::DUAL_FAST_ALT,100),
_Xparams(Params::DUAL_CP2_XBAND,100),
_collator(5000),
_statsUpdateInterval(5),
_processSband(true),
_processXband(true)
{
	_dataGramPort	= 3100;
	for (int i = 0; i < 3; i++) {
		_pulseCount[i]	= 0;
		_prevPulseCount[i] = 0;
		_errorCount[i] = 0;
		_eof[i] = false;
		_lastPulseNum[i] = 0;
	}
	_sBeamCount = 0;
	_xBeamCount = 0;

	_pSmomentThread = new MomentThread(Params::DUAL_FAST_ALT, 100);
	_pXmomentThread = new MomentThread(Params::DUAL_CP2_XBAND, 100);

	_pSmomentThread->start();

	_pXmomentThread->start();

	// intialize the data reception socket.
	// set up the ocket notifier and connect it
	// to the data reception slot
	initializeSocket();	

	// start the statistics timer
	startTimer(_statsUpdateInterval*1000);

}

/////////////////////////////////////////////////////////////////////
CP2Moments::~CP2Moments()
{

}

/////////////////////////////////////////////////////////////////////
void
CP2Moments::stopSlot(bool v)
{
	if (v) {
	}
}
/////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
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
CP2Moments::initializeSocket()	
{
	_pSocket = new QSocketDevice(QSocketDevice::Datagram);

	QHostAddress qHost;

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
	_pSocketBuf = new char[1000000];

	std::string myIPname = nameBuf;
	std::string myIPaddress = inet_ntoa(*(struct in_addr*)pHostEnt->h_addr_list[0]);
	std::cout << "ip name: " << myIPname.c_str() << ", id address  " << myIPaddress.c_str() << std::endl;

	//	m_pTextIPname->setText(myIPname.c_str());

	//	m_pTextIPaddress->setNum(+_dataGramPort);	// diagnostic print
	qHost.setAddress(myIPaddress.c_str());

	std::cout << "qHost:" << qHost.toString() << std::endl;
	std::cout << "datagram port:" << _dataGramPort << std::endl;

	if (!_pSocket->bind(qHost, _dataGramPort)) {
		qWarning("Unable to bind to %s:%d", qHost.toString().ascii(), _dataGramPort);
		exit(1); 
	}
	int sockbufsize = CP2PPI_RCVBUF;

	int result = setsockopt (_pSocket->socket(),
		SOL_SOCKET,
		SO_RCVBUF,
		(char *) &sockbufsize,
		sizeof sockbufsize);
	if (result) {
		qWarning("Set receive buffer size for socket failed");
		exit(1); 
	}

	_pSocketNotifier = new QSocketNotifier(_pSocket->socket(), QSocketNotifier::Read);

	connect(_pSocketNotifier, SIGNAL(activated(int)), this, SLOT(newDataSlot(int)));
}
//////////////////////////////////////////////////////////////////////
void 
CP2Moments::newDataSlot(int)
{
	int	readBufLen = _pSocket->readBlock((char *)_pSocketBuf, sizeof(short)*1000000);

	if (readBufLen > 0) {
		// put this datagram into a packet
		bool packetBad = _pulsePacket.setPulseData(readBufLen, _pSocketBuf);

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
						// do all of the heavy lifting for this pulse
						processPulse(pPulse);
						// sanity check on channel
						_pulseCount[chan]++;
						if (pPulse->header.status & PIRAQ_FIFO_EOF) {
							switch (chan) 
							{
							case 0:
								if (!_eof[0]) {
									_eof[0] = true;
									//									_chan0led->setBackgroundColor(QColor("red"));
								}
								break;
							case 1:
								if (!_eof[1]) {
									_eof[1] = true;
									//									_chan1led->setBackgroundColor(QColor("red"));
								}
								break;
							case 2:
								if (!_eof[2]) {
									_eof[2] = true;
									//									_chan2led->setBackgroundColor(QColor("red"));
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

	// look for pulse number errors
	int chan = pPulse->header.channel;

	if (_lastPulseNum[chan]) {
		if (_lastPulseNum[chan]+1 != pPulse->header.pulse_num) {
			_errorCount[chan]++;
		}
	}
	_lastPulseNum[chan] = pPulse->header.pulse_num;

	// beam will point to computed moments when they are ready,
	// or null if not ready
	Beam* pBeam = 0;

	// S band pulses: are successive coplaner H and V pulses
	// this horizontal switch is a hack for now; we really
	// need to find the h/v flag in the pulse header.
	if (chan == 0) 
	{	
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
CP2Moments::sDatagram(Beam* pBeam)
{
	int gates = pBeam->getNGatesOut();

	_sProductPacket.clear();
	_sProductData.resize(gates);

	CP2ProductHeader header;
	header.gates = gates;
	header.antAz = pBeam->getAz();
	header.antEl = pBeam->getEl();
	const Fields* fields = pBeam->getFields();

	header.prodType = PROD_S_DBMHC;	///< S-band dBm horizontal co-planar
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].dbmhc;  }
	_sProductPacket.addProduct(header, gates, &_sProductData[0]);

	header.prodType = PROD_S_DBMVC;	///< S-band dBm vertical co-planar
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].dbmvc;  }
	_sProductPacket.addProduct(header, gates, &_sProductData[0]);

	header.prodType = PROD_S_DBZHC;	///< S-band dBz horizontal co-planar
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].dbzhc;  }
	_sProductPacket.addProduct(header, gates, &_sProductData[0]);

	header.prodType = PROD_S_DBZVC;	///< S-band dBz vertical co-planar
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].dbzvc;  }
	_sProductPacket.addProduct(header, gates, &_sProductData[0]);

	header.prodType = PROD_S_RHOHV;	///< S-band rhohv
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].rhohv;  }
	_sProductPacket.addProduct(header, gates, &_sProductData[0]);

	header.prodType = PROD_S_PHIDP;	///< S-band phidp
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].phidp;  }
	_sProductPacket.addProduct(header, gates, &_sProductData[0]);

	header.prodType = PROD_S_ZDR;	///< S-band zdr
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].zdr;  }
	_sProductPacket.addProduct(header, gates, &_sProductData[0]);

	header.prodType = PROD_S_WIDTH;	///< S-band spectral width
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].width;  }
	_sProductPacket.addProduct(header, gates, &_sProductData[0]);

	header.prodType = PROD_S_VEL;		///< S-band velocity
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].vel;    }
	_sProductPacket.addProduct(header, gates, &_sProductData[0]);

	header.prodType = PROD_S_SNR;		///< S-band SNR
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].snr;    }
	_sProductPacket.addProduct(header, gates, &_sProductData[0]);

}
/////////////////////////////////////////////////////////////////////
void 
CP2Moments::xDatagram(Beam* pBeam)
{
	int gates = pBeam->getNGatesOut();

	_xProductPacket.clear();
	_xProductData.resize(gates);

	CP2ProductHeader header;
	header.gates = gates;
	header.antAz = pBeam->getAz();
	header.antEl = pBeam->getEl();
	const Fields* fields = pBeam->getFields();

	header.prodType = PROD_X_DBMHC;
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].dbmhc;}
	_xProductPacket.addProduct(header, gates, &_xProductData[0]);

	header.prodType =  PROD_X_DBZHC;	///< X-band dBz horizontal co-planar
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].dbzhc;  } 
	_xProductPacket.addProduct(header, gates, &_xProductData[0]);

	header.prodType = PROD_X_SNR;		///< X-band SNR
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].snr;    }
	_xProductPacket.addProduct(header, gates, &_xProductData[0]);

	header.prodType = PROD_X_LDR;		///< X-band LDR
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].ldrh;   }
	_xProductPacket.addProduct(header, gates, &_xProductData[0]);

}
/////////////////////////////////////////////////////////////////////
