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
#include <qwidgetstack.h>
#include <qcheckbox.h>

#include <iostream>
#include <algorithm>

#include <winsock.h>
#include <iostream>
#include <time.h>

CP2PPI::CP2PPI():
CP2PPIBase(),
_gates(1000),
_pSocket(0),    
_pSocketNotifier(0),
_pSocketBuf(0),	
_statsUpdateInterval(5),
_currentSbeamNum(0),
_currentXbeamNum(0),
_pause(false),
_ppiSactive(true)
{
	_dataGramPort	= 3200;
	for (int i = 0; i < 3; i++) {
		_pulseCount[i]	= 0;
	}

	// intialize the data reception socket.
	// set up the ocket notifier and connect it
	// to the data reception slot
	initializeSocket();	

	// initialize the book keeping for the plots.
	// This also sets up the radio buttons 
	// in the plot type tab widget
	initPlots();

	// count the number ppi types that initPlots 
	// gave us
	_nVarsSband = _sMomentsList.size();
	_nVarsXband = _xMomentsList.size();

	// set the intial plot type
	ppiTypeSlot(PROD_S_DBZHC);

	// create the Sband color maps
	std::set<PRODUCT_TYPES>::iterator pSet;
	_mapsSband.resize(_nVarsSband);
	for (pSet=_sMomentsList.begin(); pSet!=_sMomentsList.end(); pSet++) 
	{
		double scaleMin = _ppiInfo[*pSet].getScaleMin();
		double scaleMax = _ppiInfo[*pSet].getScaleMax();
		int index = _ppiInfo[*pSet].getPpiIndex();
		_mapsSband[index] = new ColorMap(scaleMin, scaleMax);
	}

	// create the Xband color maps
	_mapsXband.resize(_nVarsXband);
	for (pSet=_xMomentsList.begin(); pSet!=_xMomentsList.end(); pSet++) 
	{
		double scaleMin = _ppiInfo[*pSet].getScaleMin();
		double scaleMax = _ppiInfo[*pSet].getScaleMax();
		int index = _ppiInfo[*pSet].getPpiIndex();
		_mapsXband[index] = new ColorMap(scaleMin, scaleMax);
	}

	// Note that the following call determines whether PPI will 
	// use preallocated or dynamically allocated beams. If a third
	// parameter is specifiec, it will set the number of preallocated
	// beams.
	// The configure must be called after initPlots(), bcause
	// that is when _nVarsSband and _nVarsXband are determined.
	_ppiS->configure(_nVarsSband, _gates, 360);
	_ppiX->configure(_nVarsXband, _gates, 360);

	// allocate the beam data arrays
	_beamSdata.resize(_nVarsSband);
	for (int i = 0; i < _nVarsSband; i++) {
		_beamSdata[i].resize(_gates);
	}

	_beamXdata.resize(_nVarsXband);
	for (int i = 0; i < _nVarsXband; i++) {
		_beamXdata[i].resize(_gates);
	}

	QVBoxLayout* colorBarLayout = new QVBoxLayout(frameColorBar);
	_colorBar = new ColorBar(frameColorBar);
	_colorBar->configure(*_mapsSband[0]);
	colorBarLayout->addWidget(_colorBar);

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

	for (int i = 0; i < _mapsSband.size(); i++)
		delete _mapsSband[i];
}
//////////////////////////////////////////////////////////////////////
void 
CP2PPI::newDataSlot(int)
{
	int	readBufLen = _pSocket->readBlock((char *)_pSocketBuf, sizeof(short)*1000000);

	if (readBufLen > 0) {
		// put this datagram into a packet
		bool packetBad = packet.setProductData(readBufLen, _pSocketBuf);

		// Extract the products and process them.
		// From here on out, we are divorced from the
		// data transport.
		if (!packetBad) {
			for (int i = 0; i < packet.numProducts(); i++) {
				CP2Product* pProduct = packet.getProduct(i);
				// do all of the heavy lifting for this pulse
				processProduct(pProduct);
			} 
		}
	} else {
		// read error. What should we do here?
	}
}
//////////////////////////////////////////////////////////////////////
void
CP2PPI::processProduct(CP2Product* pProduct) 
{

	// fill _productSdata and _productXdata
	// from the incoming product. Once we
	// have collected all products fro either S band
	// or X band, display the beam.
	PRODUCT_TYPES prodType = pProduct->header.prodType;
	long long beamNum = pProduct->header.beamNum;
	int gates = pProduct->header.gates;
	double az = pProduct->header.antAz;
	double el = pProduct->header.antEl;

	if (_sMomentsList.find(prodType) != _sMomentsList.end()) {
		// see if the number of gates has changed
		if (gates != _beamSdata[0].size()) {
			for (int i = 0; i < _nVarsSband; i++) {
				_beamSdata[i].resize(_gates);
			}
		}
		// product is one we want for S band
		if (beamNum != _currentSbeamNum) {
			// beam number has changed; start fresh
			_currentSbeamNum = beamNum;
			_currentSproducts.clear();
		}
		_currentSproducts.insert(prodType);
		int index = _ppiInfo[prodType].getPpiIndex();
		for (int i = 0; i < gates; i++) {
			_beamSdata[index][i] = pProduct->data[i];
		}
		if (_sMomentsList.size() == _currentSproducts.size()) {
			// all products have been collected, display the beam
			displaySbeam(az, el);
			// reset the beam number tracking so that we 
			// will start a new colection of products
			_currentSbeamNum = 0;
		}
	} else {
		if (_xMomentsList.find(prodType) != _xMomentsList.end()) {
			// product is one that we want for X band
			// see if the number of gates has changed
			if (gates != _beamXdata[0].size()) {
				for (int i = 0; i < _nVarsXband; i++) {
					_beamXdata[i].resize(_gates);
				}
			}
			// product is one we want for S band
			if (beamNum != _currentXbeamNum) {
				// beam number has changed; start fresh
				_currentXbeamNum = beamNum;
				_currentXproducts.clear();
			}
			_currentXproducts.insert(prodType);
			int index = _ppiInfo[prodType].getPpiIndex();
			for (int i = 0; i < gates; i++) {
				_beamXdata[index][i] = pProduct->data[i];
			}
			if (_xMomentsList.size() == _currentXproducts.size()) {
				// all products have been collected, display the beam
				displayXbeam(az, el);
				// reset the beam number tracking so that we 
				// will start a new colection of products
				_currentXbeamNum = 0;
			}
		} else {
			// unwanted product
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

	m_pTextIPname->setText(myIPname.c_str());

	m_pTextIPaddress->setNum(+_dataGramPort);	// diagnostic print
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
////////////////////////////////////////////////////////////////////
void
CP2PPI::ppiTypeSlot(int newPpiType)
{
	if (newPpiType == _ppiSType) {
		return;
	}

	_ppiSType  = (PRODUCT_TYPES)newPpiType;
	int index = _ppiInfo[_ppiSType].getPpiIndex();
	if (_sMomentsList.find(_ppiSType)!=_sMomentsList.end()){
		_ppiS->selectVar(index);
		_colorBar->configure(*_mapsSband[index]);
	} else {
		_ppiX->selectVar(index);
		_colorBar->configure(*_mapsXband[index]);
	}
}
////////////////////////////////////////////////////////////////////
void
CP2PPI::initPlots()
{

	// set the initial plot type
	_ppiSType = PROD_S_DBZHC;

	_sMomentsList.insert(PROD_S_DBZHC);
	_sMomentsList.insert(PROD_S_DBZVC);
	_sMomentsList.insert(PROD_S_WIDTH);
	_sMomentsList.insert(PROD_S_VEL);
	_sMomentsList.insert(PROD_S_SNR);
	_sMomentsList.insert(PROD_S_RHOHV);
	_sMomentsList.insert(PROD_S_PHIDP);
	_sMomentsList.insert(PROD_S_ZDR);

	_xMomentsList.insert(PROD_X_DBZHC);
	_xMomentsList.insert(PROD_X_SNR);
	_xMomentsList.insert(PROD_X_LDR);

	int ppiVarIndex = 0;
	_ppiInfo[PROD_S_DBZHC]       = PpiInfo(PROD_S_DBZHC,      "H Dbz", "Sh: Dbz",     -20.0,  60.0,   ppiVarIndex++);
	_ppiInfo[PROD_S_DBZVC]       = PpiInfo(PROD_S_DBZVC,      "V Dbz", "Sv: Dbz",     -20.0,  60.0,   ppiVarIndex++);
	_ppiInfo[PROD_S_WIDTH]       = PpiInfo(PROD_S_WIDTH,      "Width", "S:  Width",     0.0,   5.0,   ppiVarIndex++);
	_ppiInfo[PROD_S_VEL]         = PpiInfo(  PROD_S_VEL,   "Velocity", "S:  Velocity",-20.0,  20.0,   ppiVarIndex++);
	_ppiInfo[PROD_S_SNR]         = PpiInfo(  PROD_S_SNR,        "SNR", "S:  SNR",     100.0, 150.0,   ppiVarIndex++);
	_ppiInfo[PROD_S_RHOHV]       = PpiInfo(PROD_S_RHOHV,      "Rhohv", "S:  Rhohv",     0.0,  50.0,   ppiVarIndex++);
	_ppiInfo[PROD_S_PHIDP]       = PpiInfo(PROD_S_PHIDP,      "Phidp", "S:  Phidp",     0.0, 360.0,   ppiVarIndex++);
	_ppiInfo[PROD_S_ZDR]         = PpiInfo(  PROD_S_ZDR,        "Zdr", "S:  Zdr",     -70.0,   0.0,   ppiVarIndex++);
	// restart the X band ppi indices at 0
	ppiVarIndex = 0;
	_ppiInfo[PROD_X_DBZHC]       = PpiInfo(PROD_X_DBZHC,      "H Dbz", "Xh: Dbz",     -20.0,  60.0,   ppiVarIndex++);
	_ppiInfo[PROD_X_SNR]         = PpiInfo(  PROD_X_SNR,        "SNR", "Xh: SNR",     230.0, 260.0,   ppiVarIndex++);
	_ppiInfo[PROD_X_LDR]         = PpiInfo(  PROD_X_LDR,        "LDR", "Xhv:LDR",       0.0,   1.0,   ppiVarIndex++);

	// remove the one tab that was put there by designer
	_typeTab->removePage(_typeTab->page(0));

	// add tabs, and save the button group for
	// for each tab.
	QButtonGroup* pGroup;

	pGroup = addPlotTypeTab("S", _sMomentsList);
	_tabButtonGroups.push_back(pGroup);

	pGroup = addPlotTypeTab("X", _xMomentsList);
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

	if (pageNum == 0) {
		_ppiStack->raiseWidget(0);
		_ppiSactive = true;
	} else {
		_ppiStack->raiseWidget(1);
		_ppiSactive = false;
	}
}

//////////////////////////////////////////////////////////////////////
QButtonGroup*
CP2PPI::addPlotTypeTab(std::string tabName, std::set<PRODUCT_TYPES> types)
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

	std::set<PRODUCT_TYPES>::iterator i;

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
	connect(pGroup, SIGNAL(released(int)), this, SLOT(ppiTypeSlot(int)));

	return pGroup;
}
//////////////////////////////////////////////////////////////////////
void
CP2PPI::timerEvent(QTimerEvent*) 
{

}


///////////////////////////////////////////////////////////////////////

void CP2PPI::zoomInSlot()
{
	_ppiS->setZoom(2.0);
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::zoomOutSlot()
{
	_ppiS->setZoom(0.5);
}

///////////////////////////////////////////////////////////////////////

void CP2PPI::panSlot(int panIndex)
{
	switch (panIndex) {
	case 0:
		_ppiS->setZoom(0.0);
		break;
	case 1:
		_ppiS->pan(0.0, 0.10);
		break;
	case 2:
		_ppiS->pan(-0.10, 0.10);
		break;
	case 3:
		_ppiS->pan(-0.10, 0.0);
		break;
	case 4:
		_ppiS->pan(-0.10, -0.10);
		break;
	case 5:
		_ppiS->pan(00, -0.10);
		break;
	case 6:
		_ppiS->pan(0.10, -0.10);
		break;
	case 7:
		_ppiS->pan(0.10, 0.0);
		break;
	case 8:
		_ppiS->pan(0.10, 0.10);
		break;
	default:
		break;
	}
}
////////////////////////////////////////////////
void
CP2PPI::pauseSlot(bool flag)
{
	_pause = flag;
}
//////////////////////////////////////////////////////////////////////
void
CP2PPI::displaySbeam(double az, double el)
{
	if (!_pause) {
		int gates = _beamSdata[0].size();
		_ppiS->addBeam(az - 0.5, az + 0.5, gates, _beamSdata, 1, _mapsSband);
		if (_ppiSactive)
			_azLCD->display((int)az);
	}
}
//////////////////////////////////////////////////////////////////////
void 
CP2PPI::displayXbeam(double az, double el)
{
	if (!_pause) {
		int gates = _beamXdata[0].size();
		_ppiX->addBeam(az - 0.5, az + 0.5, gates, _beamXdata, 1, _mapsXband);
		if (!_ppiSactive)
			_azLCD->display((int)az);
	}
}

//////////////////////////////////////////////////////////////////////
void
CP2PPI::doSslot(bool checked)
{
}
void
CP2PPI::doXslot(bool checked)
{
}


//////////////////////////////////////////////////////////////////////
