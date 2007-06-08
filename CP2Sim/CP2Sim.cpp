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
  _run(true)  // will be inverted by call to startStopSlot() 
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
  _pCP2SimThread->start();

  // set the run status
  startStopSlot();

  // connect the start button
  connect(_startStopButton, SIGNAL(released()), this, SLOT(startStopSlot()));

  // initialize the statistics
  timerEvent(0);

  // start the statistics timer
  startTimer(1000);

}
/////////////////////////////////////////////////////////////////////
CP2Sim::~CP2Sim()
{
  _pCP2SimThread->end();
  _pCP2SimThread->wait();
  delete _pCP2SimThread;
}
/////////////////////////////////////////////////////////////////////
void
CP2Sim::startStopSlot()
{
  // invert current state
  _run = !_run;
  // set the button text to the opposite of the
  // current state.
  if (!_run) {
    _startStopButton->setText("Start");
  } else {
    _startStopButton->setText("Stop");
  }

  // signal the thread
  _pCP2SimThread->setRunState(_run);

}
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
void
CP2Sim::timerEvent(QTimerEvent*) 
{
  double az;
  double el;
  unsigned int sweep;
  unsigned int volume;

  _pulseCount->setNum(_pCP2SimThread->getPulseCount()/1000);

}
/////////////////////////////////////////////////////////////////////
