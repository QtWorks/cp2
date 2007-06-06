#include <QNetworkInterface>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QMessageBox>
#include <QIcon>
#include <QPixmap>

#include "CP2Sim.h"
#include "CP2Net.h"
#include "CP2Version.h"
using namespace CP2Lib;

#include <iostream>

/////////////////////////////////////////////////////////////////////////////

CP2Sim::CP2Sim(QDialog* parent):
QDialog(parent),
_config("NCAR", "CP2Sim"),
_run(true)  // run will be inverted by a call to startStopSlot
{

	// setup our form
	setupUi(parent);
	// this->setWindowIcon(QIcon(QPixmap(icon)));

	// get our title from the coniguration
	std::string title = _config.getString("Title","CP2Sim");
	title += " ";
	title += CP2Version::revision();
	parent->setWindowTitle(title.c_str());

	// get the incoming and outgoing port numbers.
	int _pulsePort	    = _config.getInt("Network/PulsePort", 3100);

	// get the network identifiers
	std::string pulseNetwork   = _config.getString("Network/PulseNetwork", "192.168.1");

	// set the socket info displays
	_outIpText->setText(pulseNetwork.c_str());
	_outPortText->setNum(_pulsePort);

	// create the pulse creation thread
	_pCP2SimThread = new CP2SimThread();

	// set the run state to false
	startStopSlot();

	// connect the start button
	connect(_startStopButton, SIGNAL(released()), this, SLOT(startStopSlot()));

}
/////////////////////////////////////////////////////////////////////
CP2Sim::~CP2Sim()
{
	delete _pCP2SimThread;
}
/////////////////////////////////////////////////////////////////////
void
CP2Sim::startStopSlot()
{
	// When the button is pushed in, we are stopped
	_run = !_run;
	// set the button text to the opposite of the
	// current state.
	if (!_run) {
		_startStopButton->setText("Start");
		_statusText->setText("Stopped");
	} else {
		_startStopButton->setText("Stop");
		_statusText->setText("Running");
	}

	// signal the thread
	_pCP2SimThread->setRunState(_run);

}
//////////////////////////////////////////////////////////////////////
