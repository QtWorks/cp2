#include "CP2PPI.h"
#include <PPI/PPI.h>
#include "CP2PPI.h"
#include <qlabel.h>
#include <qtimer.h>
#include <qspinbox.h>	
#include <qlcdnumber.h>
#include <qslider.h>
#include <qlayout.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qvbox.h>
#include <qframe.h>
#include <qpushbutton.h>
#include <qpalette.h>

#include <iostream>
#include <algorithm>

#include <winsock.h>
#include <iostream>
#include <time.h>

CP2PPI::CP2PPI():
CP2PPIBase(),
		   _nVars(17),
		   _gates(1000),
		   _pSocket(0),    
		   _pSocketNotifier(0),
		   _pSocketBuf(0),	
		   _Sparams(Params::DUAL_FAST_ALT),
		   _Xparams(Params::DUAL_CP2_XBAND),
		   _collator(1000),
		   _statsUpdateInterval(5)
{
	_dataGramPort	= 3100;
	for (int i = 0; i < 3; i++) {
		_pulseCount[i]	= 0;
		_prevPulseCount[i] = 0;
		_errorCount[i] = 0;
		_eof[i] = false;
	}

	_ppiType = S_DBZHC;

	// intialize the data reception socket.
	// set up the ocket notifier and connect it
	// to the data reception slot
	initializeSocket();	

	// initialize the book keeping for the plots.
	// This also sets up the radio buttons 
	// in the plot type tab widget
	initPlots();

	// set leds to green
	//_chan0led->setBackgroundColor(QColor("green"));
	//_chan1led->setBackgroundColor(QColor("green"));
	//_chan2led->setBackgroundColor(QColor("green"));

	// set the intial plot type
	ppiTypeSlot(S_DBZHC);

	// create the color maps
	for (int i = 0; i < _nVars; i++) {
		_maps.push_back(new ColorMap(0.0, 100.0/(i+1)));
	}

	QVBoxLayout* colorBarLayout = new QVBoxLayout(frameColorBar);
	_colorBar = new ColorBar(frameColorBar);
	_colorBar->configure(*_maps[0]);
	colorBarLayout->addWidget(_colorBar);

	// Note that the following call determines whether PPI will 
	// use preallocated or dynamically allocated beams. If a third
	// parameter is specifiec, it will set the number of preallocated
	// beams.
	_ppi->configure(_nVars, _gates, 360);

	_beamData.resize(_nVars);

	// create the moment compute engine for S band
	_momentsSCompute = new MomentsCompute(_Sparams);

	// create the moment compute engine for X band
	_momentsXCompute = new MomentsCompute(_Xparams);

	// start the statistics timer
	startTimer(_statsUpdateInterval*1000);

}


///////////////////////////////////////////////////////////////////////

CP2PPI::~CP2PPI() 
{
	if (_pSocketNotifier)
		delete _pSocketNotifier;

	if (_pSocket)
		delete _pSocket;

	if (_pSocketBuf)
		delete [] _pSocketBuf;

	delete _momentsSCompute;

	delete _momentsXCompute;

	for (int i = 0; i < _maps.size(); i++)
		delete _maps[i];
}
//////////////////////////////////////////////////////////////////////
void 
CP2PPI::dataSocketActivatedSlot(int)
{
	CP2Packet packet;
	int	readBufLen = _pSocket->readBlock((char *)_pSocketBuf, sizeof(short)*1000000);

	if (readBufLen > 0) {
		// put this datagram into a packet
		bool packetBad = packet.setData(readBufLen, _pSocketBuf);

		// Extract the pulses and process them.
		// Observe paranoia for validating packets and pulses.
		// From here on out, we are divorced from the
		// data transport.
		if (!packetBad) {
			for (int i = 0; i < packet.numPulses(); i++) {
				CP2Pulse* pPulse = packet.getPulse(i);
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
CP2PPI::processPulse(CP2Pulse* pPulse) 
{
	static int outCount = 0;
	float* data = &(pPulse->data[0]);

	// beam will point to computed moments when they are ready,
	// or null if not ready
	Beam* pBeam = 0;

	PpiInfo* pi = &_ppiInfo[_ppiType];

	// S band pulses: are successive coplaner H and V pulses
	// this horizontal switch is a hack for now; we really
	// need to find the h/v flag in the pulse header.
	if (pPulse->header.channel == 0) {
		if (_sMomentsPPIs.find(_ppiType) != _sMomentsPPIs.end())
		{
			_momentsSCompute->processPulse(data,
				0,
				pPulse->header.gates,
				1.0e-6, 
				pPulse->header.antEl, 
				pPulse->header.antAz, 
				pPulse->header.pulse_num,
				pPulse->header.horiz);	
			// ask for a completed beam. The return value will be
			// 0 if nothing is ready yet.
			pBeam = _momentsSCompute->getNewBeam();
		} else {
			pBeam = 0;
		}
		if (pBeam) {
			// we have S band products
			const Fields* fields = pBeam->getFields();
			}
	} else {
		if (_sMomentsPPIs.find(_ppiType)==_sMomentsPPIs.end())
		{
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
			_collator.addPulse(pFullPulse, pFullPulse->header()->channel - 1);

			// now see if we have some matching pulses
			CP2FullPulse* pHPulse;
			CP2FullPulse* pVPulse;
			if (_collator.gotMatch(&pHPulse, &pVPulse)) {
				// a matching pair was found. Send them to the X band
				// moments compute engine.
				_momentsXCompute->processPulse(
					pHPulse->data(), 
					pVPulse->data(),
					pPulse->header.gates,
					1.0e-6,
					pPulse->header.antEl, 
					pPulse->header.antAz, 
					pPulse->header.pulse_num,
					true);	
				// finished with these pulses, so delete them.
				delete pHPulse;
				delete pVPulse;
				// ask for a completed beam. The return value will be
				// 0 if nothing is ready yet.
				pBeam = _momentsXCompute->getNewBeam();
			}else {
				pBeam = 0;
			}
			if (pBeam) {
				// we have X products
				const Fields* fields = pBeam->getFields();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
void
CP2PPI::initializeSocket()	
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
	int sockbufsize = 1000000;

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

	connect(_pSocketNotifier, SIGNAL(activated(int)), this, SLOT(dataSocketActivatedSlot(int)));
}
////////////////////////////////////////////////////////////////////
void
CP2PPI::ppiTypeSlot(int newPlotType)
{
	if (newPlotType == _ppiType) {
		return;
	}
}
////////////////////////////////////////////////////////////////////
void
CP2PPI::initPlots()
{

	_sMomentsPPIs.insert(S_DBMHC);
	_sMomentsPPIs.insert(S_DBMVC);
	_sMomentsPPIs.insert(S_DBZHC);
	_sMomentsPPIs.insert(S_DBZVC);
	_sMomentsPPIs.insert(S_WIDTH);
	_sMomentsPPIs.insert(S_VEL);
	_sMomentsPPIs.insert(S_SNR);
	_sMomentsPPIs.insert(S_RHOHV);
	_sMomentsPPIs.insert(S_PHIDP);
	_sMomentsPPIs.insert(S_ZDR);

	_xMomentsPPIs.insert(X_DBMHC);
	_xMomentsPPIs.insert(X_DBMVX);
	_xMomentsPPIs.insert(X_DBZHC);
	_xMomentsPPIs.insert(X_SNR);
	//	_xMomentsPPIs.insert(X_WIDTH);
	//	_xMomentsPPIs.insert(X_VEL);
	_xMomentsPPIs.insert(X_LDR);

	_ppiInfo[S_DBMHC]       = PpiInfo(      S_DBMHC,   "H Dbm", "Sh: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[S_DBMVC]       = PpiInfo(      S_DBMVC,   "V Dbm", "Sv: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[S_DBZHC]       = PpiInfo(      S_DBZHC,   "H Dbz", "Sh: Dbz", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[S_DBZVC]       = PpiInfo(      S_DBZVC,   "V Dbz", "Sv: Dbz", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[S_WIDTH]       = PpiInfo(      S_WIDTH,   "Width", "S:  Width", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[S_VEL]         = PpiInfo(        S_VEL,   "Velocity", "S:  Velocity", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[S_SNR]         = PpiInfo(        S_SNR,   "SNR", "S:  SNR", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[S_RHOHV]       = PpiInfo(      S_RHOHV,   "Rhohv", "S:  Rhohv", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[S_PHIDP]       = PpiInfo(      S_RHOHV,   "Phidp", "S:  Phidp", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[S_ZDR]         = PpiInfo(      S_ZDR,     "Zdr", "S:  Zdr", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[X_DBMHC]       = PpiInfo(      X_DBMHC,   "H Dbm", "Xh: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[X_DBMVX]       = PpiInfo(      X_DBMVX,   "V Cross Dbm", "Xv: Dbm", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[X_DBZHC]       = PpiInfo(      X_DBZHC,   "H Dbz", "Xh: Dbz", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[X_SNR]         = PpiInfo(        X_SNR,   "SNR", "Xh: SNR", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[X_WIDTH]       = PpiInfo(      X_WIDTH,   "Width", "Xh: Width", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[X_VEL]         = PpiInfo(        X_VEL,   "Velocity", "Xh: Velocity", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);
	_ppiInfo[X_LDR]         = PpiInfo(        X_LDR,   "LDR", "Xhv:LDR", -5.0, 5.0, 0.0, -5.0, 5.0, 0.0);

	// remove the one tab that was put there by designer
	_typeTab->removePage(_typeTab->page(0));

	// add tabs, and save the button group for
	// for each tab.
	QButtonGroup* pGroup;

	pGroup = addPlotTypeTab("S", _sMomentsPPIs);
	_tabButtonGroups.push_back(pGroup);

	pGroup = addPlotTypeTab("X", _xMomentsPPIs);
	_tabButtonGroups.push_back(pGroup);

	connect(_typeTab, SIGNAL(currentChanged(QWidget *)), 
		this, SLOT(tabChangeSlot(QWidget*)));
}
//////////////////////////////////////////////////////////////////////
void
CP2PPI::tabChangeSlot(QWidget* w) 
{
	// find out the index of the current page
	int pageNum = _typeTab->currentPageIndex();

	// get the radio button id of the currently selected button
	// on that page.
	int plotType = _tabButtonGroups[pageNum]->selectedId();

	// change the plot type
	ppiTypeSlot(plotType);

}

//////////////////////////////////////////////////////////////////////
QButtonGroup*
CP2PPI::addPlotTypeTab(std::string tabName, std::set<PPITYPE> types)
{
	QWidget* pPage;
	QButtonGroup* pGroup;
	QVBoxLayout* pVbox;
	QVBoxLayout* pButtonVbox;
	QRadioButton* pRadio;

	pPage = new QWidget(_typeTab, tabName.c_str());
	pVbox = new QVBoxLayout(pPage);

	pGroup = new QButtonGroup(pPage);
	pGroup->setColumnLayout(0, Qt::Vertical );
	pGroup->layout()->setSpacing( 6 );
	pGroup->layout()->setMargin( 11 );

	pButtonVbox = new QVBoxLayout(pGroup->layout());
	pButtonVbox->setAlignment( Qt::AlignTop );

	std::set<PPITYPE>::iterator i;

	for (i = types.begin(); i != types.end(); i++) 
	{
		int id = _ppiInfo[*i].getId();
		pRadio = new QRadioButton(pGroup);
		pGroup->insert(pRadio, id);
		const QString label = _ppiInfo[*i].getLongName().c_str();
		pRadio->setText(label);
		pButtonVbox->addWidget(pRadio);

		// set the first radio button of the group
		// to be selected.
		if (i == types.begin()) {
			pRadio->setChecked(true);
		}
	}

	pVbox->addWidget(pGroup);
	_typeTab->insertTab(pPage, tabName.c_str());

	// connect the button released signal to our
	// our plot type change slot.
	connect(pGroup, SIGNAL(released(int)), this, SLOT(plotTypeSlot(int)));

	return pGroup;
}
//////////////////////////////////////////////////////////////////////
void
CP2PPI::timerEvent(QTimerEvent*) 
{

	int rate[3];
	for (int i = 0; i < 3; i++) 
	{
		rate[i] = (_pulseCount[i] - _prevPulseCount[i])/_statsUpdateInterval;
		_prevPulseCount[i] = _pulseCount[i];
	}
//	_chan0pulseCount->setNum(_pulseCount[0]/1000);
//	_chan0pulseRate->setNum(rate[0]);
//	_chan0errors->setNum(0);
//	_chan1pulseCount->setNum(_pulseCount[1]/1000);
//	_chan1pulseRate->setNum(rate[1]);
//	_chan1errors->setNum(0);
//	_chan2pulseCount->setNum(_pulseCount[2]/1000);
//	_chan2pulseRate->setNum(rate[2]);
//	_chan2errors->setNum(0);
}


///////////////////////////////////////////////////////////////////////

void CP2PPI::zoomInSlot()
{
	_ppi->setZoom(2.0);
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::zoomOutSlot()
{
	_ppi->setZoom(0.5);
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::panSlot(int panIndex)
{
	switch (panIndex) {
	case 0:
		_ppi->setZoom(0.0);
		break;
	case 1:
		_ppi->pan(0.0, 0.10);
		break;
	case 2:
		_ppi->pan(-0.10, 0.10);
		break;
	case 3:
		_ppi->pan(-0.10, 0.0);
		break;
	case 4:
		_ppi->pan(-0.10, -0.10);
		break;
	case 5:
		_ppi->pan(00, -0.10);
		break;
	case 6:
		_ppi->pan(0.10, -0.10);
		break;
	case 7:
		_ppi->pan(0.10, 0.0);
		break;
	case 8:
		_ppi->pan(0.10, 0.10);
		break;
	default:
		break;
	}
}
////////////////////////////////////////////////
void
CP2PPI::startStopDisplaySlot()
{
}

