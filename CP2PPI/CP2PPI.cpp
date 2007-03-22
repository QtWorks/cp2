#include "CP2PPI.h"
#include <PPI/PPI.h>
#include "CP2PPI.h"
#include <QLabel>
#include <QTimer>
#include <QSpinbox>	
#include <QLcdNumber>
#include <QSlider>
#include <QLayout>
#include <QTabWidget>
#include <QWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QString>
#include <QFrame>
#include <QPushButton>
#include <QPalette>
#include <QStackedWidget>
#include <QCheckBox>
#include <QMessageBox>

#include "ColorBarSettings.h"

#include <iostream>
#include <algorithm>

#include <winsock.h>
#include <iostream>
#include <time.h>

CP2PPI::CP2PPI(QDialog* parent):
QDialog(parent),
_gates(10),
_pSocket(0),    
_pSocketBuf(0),	
_statsUpdateInterval(5),
_currentSbeamNum(0),
_currentXbeamNum(0),
_pause(false),
_ppiSactive(true),
_productPort(3200),
_config("NCAR", "CP2PPI")
{
	setupUi(parent);

	_config.setString("title", "CP2PPI Plan Position Index Display");

	for (int i = 0; i < 3; i++) {
		_pulseCount[i]	= 0;
	}

	// intialize the data reception socket.
	// set up the ocket notifier and connect it
	// to the data reception slot
	initSocket();	

	// initialize the book keeping for the plots.
	// This also sets up the radio buttons 
	// in the plot type tab widget
	initPlots();

	// count the number ppi types that initPlots 
	// gave us
	_nVarsSband = _sMomentsList.size();
	_nVarsXband = _xMomentsList.size();

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

	configureForGates();
	// set the intial plot type
	_ppiSType = PROD_S_DBMHC;
	ppiTypeSlot(PROD_S_DBZHC);

	// capture a user click on the color bar
	connect(_colorBar, SIGNAL(released()), this, SLOT(colorBarReleasedSlot()));

	// start the statistics timer
	startTimer(_statsUpdateInterval*1000);

}


///////////////////////////////////////////////////////////////////////

CP2PPI::~CP2PPI() 
{

	if (_pSocket)
		delete _pSocket;

	if (_pSocketBuf)
		delete [] _pSocketBuf;

	for (int i = 0; i < _mapsSband.size(); i++)
		delete _mapsSband[i];
}

//////////////////////////////////////////////////////////////////////
void
CP2PPI::configureForGates() 
{
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
}

//////////////////////////////////////////////////////////////////////
void 
CP2PPI::newDataSlot()
{
	int	readBufLen = _pSocket->readDatagram((char *)_pSocketBuf, sizeof(short)*1000000);

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
	double az = pProduct->header.az;
	az = 450 - az;
	if (az < 0) 
		az += 360.0;
	else
		if (az > 360)
			az -= 360.0;
	double el = pProduct->header.el;

	if (gates != _gates) {
		_gates = gates;
		configureForGates();
	}

	if (_sMomentsList.find(prodType) != _sMomentsList.end()) {
		// std::cout << "S product " << prodType << "   " << beamNum << "\n";
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
			// std::cout << "X product " << prodType << "   " << beamNum << "\n";
			// product is one that we want for X band
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
			// std::cout << "U product " << prodType << "   " << beamNum << "\n";			
		}
	}
}

//////////////////////////////////////////////////////////////////////
void
CP2PPI::initSocket()	
{
	// get the interface specification
	std::string interfaceNetwork;
	interfaceNetwork = _config.getString("Network/PulseNetwork", "192.168.1");
	// create the incoming product socket
	_pSocket = new CP2UdpSocket(interfaceNetwork, _productPort, false, 0, 10000000);

	if (!_pSocket->ok()) {
		QMessageBox e;
		e.warning(this, "Error",_pSocket->errorMsg().c_str(), 
			QMessageBox::Ok, QMessageBox::NoButton);
		return;
	}

	// display the socket specifics
	QString ip(_pSocket->toString().c_str());
	_textIP->setText(ip);
	_textPort->setNum(_productPort);

	_pSocketBuf = new char[1000000];

	connect(_pSocket, SIGNAL(readyRead()), this, SLOT(newDataSlot()));
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
	setPpiInfo(PROD_S_DBZHC, "S_DBZHC", "H Dbz", "Sh: Dbz",     -70.0,   0.0,   ppiVarIndex++);
	setPpiInfo(PROD_S_DBZVC, "S_DBZVC", "V Dbz", "Sv: Dbz",     -70.0,   0.0,   ppiVarIndex++);
	setPpiInfo(PROD_S_WIDTH, "S_WIDTH", "Width", "S:  Width",     0.0,   5.0,   ppiVarIndex++);
	setPpiInfo(  PROD_S_VEL, "S_VEL",   "Velocity", "S:  Velocity",-20.0,  20.0,   ppiVarIndex++);
	setPpiInfo(  PROD_S_SNR, "S_SNR",   "SNR", "S:  SNR",        -40.0,  10.0,   ppiVarIndex++);
	setPpiInfo(PROD_S_RHOHV, "S_RHOHV", "Rhohv", "S:  Rhohv",     0.0,   1.0,   ppiVarIndex++);
	setPpiInfo(PROD_S_PHIDP, "S_PHIDP", "Phidp", "S:  Phidp",    -180.0, 180.0,   ppiVarIndex++);
	setPpiInfo(  PROD_S_ZDR, "S_ZDR",   "Zdr", "S:  Zdr",        -70.0,   0.0,   ppiVarIndex++);
	// restart the X band ppi indices at 0
	ppiVarIndex = 0;
	setPpiInfo(PROD_X_DBZHC,"X_DBZHC",  "H Dbz", "Xh: Dbz",     -70.0,   0.0,   ppiVarIndex++);
	setPpiInfo(  PROD_X_SNR,"X_SNR",    "SNR", "Xh: SNR",       -40.0,   0.0,   ppiVarIndex++);
	setPpiInfo(  PROD_X_LDR,"X_LDR",    "LDR", "Xhv:LDR",         0.0,   1.0,   ppiVarIndex++);

	_typeTab->removeTab(0);
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
	int pageNum = _typeTab->currentIndex();

	// get the radio button id of the currently selected button
	// on that page.
	int plotType = _tabButtonGroups[pageNum]->checkedId();

	// change the plot type
	ppiTypeSlot(plotType);

	if (pageNum == 0) {
		_ppiStack->setCurrentIndex(0);
		_ppiSactive = true;
	} else {
		_ppiStack->setCurrentIndex(1);
		_ppiSactive = false;
	}
}

//////////////////////////////////////////////////////////////////////
QButtonGroup*
CP2PPI::addPlotTypeTab(std::string tabName, std::set<PRODUCT_TYPES> types)
{
	// The page that will be added to the tab widget
	QWidget* pPage = new QWidget;
	// the layout manager for the page, will contain the buttons
	QVBoxLayout* pVbox = new QVBoxLayout;
	// the button group manager, which has nothing to do with rendering
	QButtonGroup* pGroup = new QButtonGroup;

	std::set<PRODUCT_TYPES>::iterator i;

	for (i = types.begin(); i != types.end(); i++) 
	{
		// create the radio button
		int id = _ppiInfo[*i].getId();
		QRadioButton* pRadio = new QRadioButton;
		const QString label = _ppiInfo[*i].getLongName().c_str();
		pRadio->setText(label);

		// put the button in the button group
		pGroup->addButton(pRadio, id);
		// assign the button to the layout manager
		pVbox->addWidget(pRadio);

		// set the first radio button of the group
		// to be selected.
		if (i == types.begin()) {
			pRadio->setChecked(true);
		}
	}
	// associate the layout manager with the page
	pPage->setLayout(pVbox);

	// put the page on the tab
	_typeTab->insertTab(-1, pPage, tabName.c_str());

	// connect the button released signal to our plot type change slot.
	connect(pGroup, SIGNAL(buttonReleased(int)), this, SLOT(ppiTypeSlot(int)));

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
		if (_ppiSactive) {
			_azLCD->display((int)az);
			_elLCD->display((int)el);
		}
	}
}
//////////////////////////////////////////////////////////////////////
void 
CP2PPI::displayXbeam(double az, double el)
{
	if (!_pause) {
		int gates = _beamXdata[0].size();
		_ppiX->addBeam(az - 0.5, az + 0.5, gates, _beamXdata, 1, _mapsXband);
		if (!_ppiSactive) {
			_azLCD->display((int)az);
			_elLCD->display((int)el);
		}
	}
}
//////////////////////////////////////////////////////////////////////
void
CP2PPI::doSslot(bool checked)
{
}
//////////////////////////////////////////////////////////////////////
void
CP2PPI::doXslot(bool checked)
{
}
//////////////////////////////////////////////////////////////////////
void
CP2PPI::colorBarReleasedSlot()
{
	// get the currently selected plot type
	PRODUCT_TYPES plotType = currentProductType();

	// get the current settings
	double min = _ppiInfo[plotType].getScaleMin();
	double max = _ppiInfo[plotType].getScaleMax();

	// create the color bar settings dialog
	_colorBarSettings = new ColorBarSettings(min, max, this);

	// connect the finished slot so that the dialog status 
	// can be captuyred when the dialog closes
	connect(_colorBarSettings, SIGNAL(finished(int)), 
		this, SLOT(colorBarSettingsFinishedSlot(int)));

	// and show it
	_colorBarSettings->show();

}
//////////////////////////////////////////////////////////////////////////////
void 
CP2PPI::colorBarSettingsFinishedSlot(int result)
{
	// see if the OK button was hit
	if (result == QDialog::Accepted) {
		// get the scale values from the settings dialog
		double scaleMin = _colorBarSettings->getMinimum();
		double scaleMax = _colorBarSettings->getMaximum();

		// if the user inverted the values, swap them
		if (scaleMin > scaleMax) 
		{
			double temp = scaleMax;
			scaleMax = scaleMin;
			scaleMin = temp;
		}
		// find out what product is currently displayed
		// (it might not be the one selected when the
		// dialog was acitvated, but that's okay)
		PRODUCT_TYPES plotType = currentProductType();
		// and reconfigure the color bar
		int index = _ppiInfo[plotType].getPpiIndex();
		if (_sMomentsList.find(_ppiSType)!=_sMomentsList.end())
		{
			// make a new color map
			delete _mapsSband[index];
			_mapsSband[index] = new ColorMap(scaleMin, scaleMax);
			// configure the color bar with it
			_colorBar->configure(*_mapsSband[index]);
		} else {
			// make a new color map
			delete _mapsXband[index];
			_mapsXband[index] = new ColorMap(scaleMin, scaleMax);
			// configure the color bar with it
			_colorBar->configure(*_mapsXband[index]);
		}
		// assign the new scale values to the current product
		_ppiInfo[plotType].setScale(scaleMin, scaleMax);
		// save the new values in the configuration
		// create the configuration keys
		std::string key = _ppiInfo[plotType].getKey();
		std::string minKey = "ColorScales/";
		minKey += key;
		minKey += "_min";

		std::string maxKey = "ColorScales/";
		maxKey += key;
		maxKey += "_max";

		// get the configuration values
		_config.setDouble(minKey, scaleMin);
		_config.setDouble(maxKey, scaleMax);


	}
}
//////////////////////////////////////////////////////////////////////
PRODUCT_TYPES
CP2PPI::currentProductType()
{
	// find out the index of the current page
	int pageNum = _typeTab->currentIndex();

	// get the radio button id of the currently selected button
	// on that page.
	PRODUCT_TYPES plotType = (PRODUCT_TYPES)_tabButtonGroups[pageNum]->checkedId();

	return plotType;
}
//////////////////////////////////////////////////////////////////////
void 
CP2PPI::setPpiInfo(PRODUCT_TYPES t, 
				   std::string key,             
				   std::string shortName,       
				   std::string longName,        
				   double defaultScaleMin,      
				   double defaultScaleMax,      
				   int ppiVarIndex)
{
	// create the configuration keys
	std::string minKey = "ColorScales/";
	minKey += key;
	minKey += "_min";

	std::string maxKey = "ColorScales/";
	maxKey += key;
	maxKey += "_max";

	// get the configuration values
	double min = _config.getDouble(minKey, defaultScaleMin);
	double max = _config.getDouble(maxKey, defaultScaleMax);

	// set the ppi configuration
	_ppiInfo[t] = PpiInfo(t, key, shortName, longName, min, max, ppiVarIndex);

	_config.sync();

}

