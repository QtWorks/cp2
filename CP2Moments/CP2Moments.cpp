#include <QNetworkInterface>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QMessageBox>

#include "CP2Moments.h"
#include "CP2Net.h"
#include "CP2Version.h"

#include <Windows.h>  // just to get Sleep()
#include <iostream>

CP2Moments::CP2Moments(QDialog* parent):
QDialog(parent),
_pPulseSocket(0),    
_pPulseSocketBuf(0),	
_collator(5000),
_statsUpdateInterval(5),
_processSband(true),
_processXband(true),
_config("NCAR", "CP2Moments")
{

	fftw_init_threads();
	fftw_plan_with_nthreads(2);

	// setup our form
	setupUi(parent);

	// get our title from the coniguration
	std::string title = _config.getString("Title","CP2Moments");
	title += " ";
	title += CP2Version::revision();
	parent->setWindowTitle(title.c_str());

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

	// get the processing parameters
	// create the Sband moments processing thread
	Params Sparams;

	Sparams.system_phidp                 = _config.getDouble("ProcessingSband/system_phidp_deg",        0.0);
	Sparams.correct_for_system_phidp     = _config.getBool("ProcessingSband/correct_phidp",             false);

	Sparams.moments_params.mode                   = Params::DUAL_CP2_SBAND;
	Sparams.moments_params.gate_spacing           = _config.getDouble("ProcessingSband/gateSpacingKm",     0.150);
	Sparams.moments_params.start_range            = Sparams.moments_params.gate_spacing/2.0;
	Sparams.moments_params.n_samples              = _config.getInt("ProcessingSband/pulsesPerBeam",          100);
	Sparams.moments_params.algorithm              = Params::ALG_PP;
	Sparams.moments_params.index_beams_in_azimuth = _config.getBool("ProcessingSband/indexBeamInAz",        true);
	Sparams.moments_params.azimuth_resolution     = _config.getDouble("ProcessingSband/azResolutionDeg",     1.0);
	Sparams.moments_params.apply_clutter_filter   = _config.getBool("ProcessingSband/clutterFilterEnable", false);
	if (Sparams.moments_params.apply_clutter_filter)
		Sparams.moments_params.window = Params::WINDOW_VONHANN;
	else
		Sparams.moments_params.window = Params::WINDOW_NONE;

	Sparams.radar.horiz_beam_width                = _config.getDouble("ProcessingSband/horizBeamWidthDeg",  0.91);
	Sparams.radar.vert_beam_width                 = _config.getDouble("ProcessingSband/vertBeamWidthDeg",   0.91);
	Sparams.radar.xmit_rcv_mode                   = Params::DP_ALT_HV_CO_ONLY;

	Sparams.hc_receiver.noise_dBm        = _config.getDouble("ProcessingSband/hc_rcvr_noise_dbm",      -77.0);
	Sparams.hc_receiver.gain             = _config.getDouble("ProcessingSband/hc_rcvr_gain_db",         37.0);
	Sparams.hc_receiver.radar_constant   = _config.getDouble("ProcessingSband/hc_rcvr_radar_constant", -68.4);

	Sparams.hx_receiver.noise_dBm        = _config.getDouble("ProcessingSband/hx_rcvr_noise_dbm",      -77.0);
	Sparams.hx_receiver.gain             = _config.getDouble("ProcessingSband/hx_rcvr_gain_db",         37.0);
	Sparams.hx_receiver.radar_constant   = _config.getDouble("ProcessingSband/hx_rcvr_radar_constant", -68.4);

	Sparams.vc_receiver.noise_dBm        = _config.getDouble("ProcessingSband/vc_rcvr_noise_dbm",      -77.0);
	Sparams.vc_receiver.gain             = _config.getDouble("ProcessingSband/vc_rcvr_gain_db",         37.0);
	Sparams.vc_receiver.radar_constant   = _config.getDouble("ProcessingSband/vc_rcvr_radar_constant", -68.4);

	Sparams.vx_receiver.noise_dBm        = _config.getDouble("ProcessingSband/vx_rcvr_noise_dbm",      -77.0);
	Sparams.vx_receiver.gain             = _config.getDouble("ProcessingSband/vx_rcvr_gain_db",         37.0);
	Sparams.vx_receiver.radar_constant   = _config.getDouble("ProcessingSband/vx_rcvr_radar_constant", -68.4);

	_pSmomentThread = new MomentThread(Sparams);

	// create the Sband moments processing thread
	Params Xparams;

	Xparams.system_phidp                          = _config.getDouble("ProcessingXband/system_phidp_deg", 0.0);
	Xparams.correct_for_system_phidp              = _config.getBool("ProcessingXband/correct_phidp",      false);

	Xparams.moments_params.mode                   = Params::DUAL_CP2_XBAND;
	Xparams.moments_params.gate_spacing           = _config.getDouble("ProcessingXband/gateSpacingKm", 0.150);
	Xparams.moments_params.start_range            = Xparams.moments_params.gate_spacing/2.0;
	Xparams.moments_params.n_samples              = _config.getInt("ProcessingXband/pulsesPerBeam", 100);
	Xparams.moments_params.algorithm              = Params::ALG_PP;
	Xparams.moments_params.index_beams_in_azimuth = _config.getBool("ProcessingXband/indexBeamInAz", true);
	Xparams.moments_params.azimuth_resolution     = _config.getDouble("ProcessingXband/azResolutionDeg", 1.0);
	Xparams.moments_params.apply_clutter_filter   = _config.getBool("ProcessingXband/clutterFilterEnable", false);
	if (Xparams.moments_params.apply_clutter_filter)
		Xparams.moments_params.window = Params::WINDOW_VONHANN;
	else
		Xparams.moments_params.window = Params::WINDOW_NONE;


	Xparams.radar.horiz_beam_width       = _config.getDouble("ProcessingXband/horizBeamWidthDeg", 0.91);
	Xparams.radar.vert_beam_width        = _config.getDouble("ProcessingXband/vertBeamWidthDeg", 0.91);
	Xparams.radar.xmit_rcv_mode          = Params::DP_H_ONLY_FIXED_HV;

	Xparams.hc_receiver.noise_dBm        = _config.getDouble("ProcessingXband/hc_rcvr_noise_dbm",      -77.0);
	Xparams.hc_receiver.gain             = _config.getDouble("ProcessingXband/hc_rcvr_gain_db",         37.0);
	Xparams.hc_receiver.radar_constant   = _config.getDouble("ProcessingXband/hc_rcvr_radar_constant", -68.4);

	Xparams.hx_receiver.noise_dBm        = _config.getDouble("ProcessingXband/hx_rcvr_noise_dbm",      -77.0);
  	Xparams.hx_receiver.gain             = _config.getDouble("ProcessingXband/hx_rcvr_gain_db",         37.0);
	Xparams.hx_receiver.radar_constant   = _config.getDouble("ProcessingXband/hx_rcvr_radar_constant", -68.4);

	Xparams.vc_receiver.noise_dBm        = _config.getDouble("ProcessingXband/vc_rcvr_noise_dbm",      -77.0);
	Xparams.vc_receiver.gain             = _config.getDouble("ProcessingXband/vc_rcvr_gain_db",         37.0);
	Xparams.vc_receiver.radar_constant   = _config.getDouble("ProcessingXband/vc_rcvr_radar_constant", -68.4);

	Xparams.vx_receiver.noise_dBm        = _config.getDouble("ProcessingXband/vx_rcvr_noise_dbm",      -77.0);
	Xparams.vx_receiver.gain             = _config.getDouble("ProcessingXband/vx_rcvr_gain_db",         37.0);
	Xparams.vx_receiver.radar_constant   = _config.getDouble("ProcessingXband/vx_rcvr_radar_constant", -68.4);

	_pXmomentThread = new MomentThread(Xparams);

	// At the moment, set the gate spacing to the Sband gate spacing.
	// This really needs to be separated into X and S spacing, and accounted
	// for in CP2PPI.
	_gateSpacing = Sparams.moments_params.gate_spacing;

	// display a few of the processing parameters on the UI.
	_sPulsesPerBeam->setNum(Sparams.moments_params.n_samples);
	_sClutterFilter->setText(Sparams.moments_params.apply_clutter_filter ? "On":"Off");
	_xPulsesPerBeam->setNum(Xparams.moments_params.n_samples);
	_xClutterFilter->setText(Xparams.moments_params.apply_clutter_filter ? "On":"Off");
		
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
//		_startStopButton->setText("Start");
		_statusText->setText("Stopped");
	} else {
//		_startStopButton->setText("Stop");
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

	_piraq0Errors->setNum(_errorCount[0]);
	_piraq1Errors->setNum(_errorCount[1]);
	_piraq2Errors->setNum(_errorCount[2]);

	_piraq0rate->setNum(rate[0]);
	_piraq1rate->setNum(rate[1]);
	_piraq2rate->setNum(rate[2]);

	_sBeams->setNum(_sBeamCount);
	_xBeams->setNum(_xBeamCount);

	_collatorErrors->setNum(collatorErrors);

}
//////////////////////////////////////////////////////////////////////
void
CP2Moments::initializeSockets()	
{
		// assign the incoming and outgoing port numbers.
	_pulsePort	    = _config.getInt("Network/PulsePort", 3100);
	_productsPort   = _config.getInt("Network/ProductPort", 3200);

	// get the network identifiers
	std::string pulseNetwork   = _config.getString("Network/PulseNetwork", "192.168.1");
	std::string productNetwork = _config.getString("Network/ProductNetwork", "192.168.1");

	// allocate the buffer that will recieve the incoming pulse data
	_pPulseSocketBuf = new char[1000000];

	// create the sockets
	_pPulseSocket   = new CP2UdpSocket(pulseNetwork, _pulsePort, false, 0, CP2MOMENTS_PULSE_RCVBUF);

	if (!_pPulseSocket->ok()) {
		std::string errMsg = _pPulseSocket->errorMsg().c_str();
		errMsg += "Products will not be computed";
		QMessageBox e;
		e.warning(this, "Error", errMsg.c_str(), 
			QMessageBox::Ok, QMessageBox::NoButton);
	}

	_pProductSocket = new CP2UdpSocket(productNetwork, _productsPort, true, CP2MOMENTS_PROD_SNDBUF, 0);
	if (!_pProductSocket->ok()) {
		std::string errMsg = _pProductSocket->errorMsg().c_str();
		errMsg += "Products will not be transmitted";
		QMessageBox e;
		e.warning(this, "Error", errMsg.c_str(), 
			QMessageBox::Ok, QMessageBox::NoButton);
	}

	// set the socket info displays
	_outIpText->setText(_pProductSocket->toString().c_str());
	_outPortText->setNum(_productsPort);

	_inIpText->setText(_pPulseSocket->toString().c_str());
	_inPortText->setNum(_pulsePort);


	// set up the socket notifier for pulse datagrams
	connect(_pPulseSocket, SIGNAL(readyRead()), this, SLOT(newPulseDataSlot()));

	// The max datagram message must be smaller than 64K
	_soMaxMsgSize = 64000;

}
//////////////////////////////////////////////////////////////////////
void 
CP2Moments::newPulseDataSlot()
{
	int	readBufLen = _pPulseSocket->readDatagram(
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
					// scale the I/Q counts to something. we will probably do 
					// this eventually in cp2exec.
					float* pData = pPulse->data;
					int gates = pPulse->header.gates;
					for (int i = 0; i < 2*gates; i++) {
						pData[i]   *= PIRAQ3D_SCALE;
					}
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
			sBeamOut(pBeam);
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
			xBeamOut(pBeam);
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
		int bytesSent = 0;
		while (bytesSent != packet.packetSize()) {
			bytesSent = _pProductSocket->writeDatagram(
			(const char*)packet.packetData(),
			packet.packetSize());
			if(bytesSent != packet.packetSize())
				Sleep(1);
		}
		packet.clear();
	}

	packet.addProduct(header, data.size(), &data[0]);
	if (forceSend) {
		int bytesSent = 0;
		while (bytesSent != packet.packetSize()) {
		bytesSent = _pProductSocket->writeDatagram(
			(const char*)packet.packetData(),
			packet.packetSize());
			if(bytesSent != packet.packetSize())
				Sleep(1);
		}
		packet.clear();
	}
}
////////////////////////////////////////////////////////////////////
void 
CP2Moments::sBeamOut(Beam* pBeam)
{
	int gates = pBeam->getNGatesOut();

	_sProductPacket.clear();
	_sProductData.resize(gates);

	CP2ProductHeader header;
	header.beamNum	   = pBeam->getSeqNum();
	header.gates	   = gates;
	header.az		   = pBeam->getAz();
	header.el		   = pBeam->getEl();
	header.gateWidthKm = _gateSpacing;

	const Fields* fields = pBeam->getFields();

	header.prodType = PROD_S_DBMHC;	///< S-band dBm horizontal co-planar
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].dbmhc;  }
	sendProduct(header, _sProductData, _sProductPacket);

	header.prodType = PROD_S_DBMVC;	///< S-band dBm vertical co-planar
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].dbmvc;  }
	sendProduct(header, _sProductData, _sProductPacket);

	header.prodType = PROD_S_DBZ;	///< S-band dBz
	for (int i = 0; i < gates; i++) { _sProductData[i] = fields[i].dbz;  }
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
CP2Moments::xBeamOut(Beam* pBeam)
{
	int gates = pBeam->getNGatesOut();

	_xProductPacket.clear();
	_xProductData.resize(gates);

	CP2ProductHeader header;
	header.beamNum     = pBeam->getSeqNum();
	header.gates       = gates;
	header.az          = pBeam->getAz();
	header.el          = pBeam->getEl();
	header.gateWidthKm = _gateSpacing;

	const Fields* fields = pBeam->getFields();

	header.prodType = PROD_X_DBMHC;
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].dbmhc;}
	sendProduct(header, _xProductData, _xProductPacket);

	header.prodType = PROD_X_DBMVX;
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].dbmvx;}
	sendProduct(header, _xProductData, _xProductPacket);

	header.prodType =  PROD_X_DBZ;	   ///< X-band dBz
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].dbz;  } 
	sendProduct(header, _xProductData, _xProductPacket);

	header.prodType = PROD_X_SNR;		///< X-band SNR
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].snr;    }
	sendProduct(header, _xProductData, _xProductPacket);

	header.prodType = PROD_X_LDR;		///< X-band LDR
	for (int i = 0; i < gates; i++) { _xProductData[i] = fields[i].ldrh;   }
	sendProduct(header, _xProductData, _xProductPacket, true);

}
