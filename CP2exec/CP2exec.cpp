#include "CP2Exec.h"
#include <qlabel.h>

CP2Exec::CP2Exec(QDialog* parent, std::string dspObjFile, std::string configFile):
QDialog(parent),
_dspObjFile(dspObjFile),
_configFile(configFile),
_statsUpdateInterval(5),
_eofLed0(false),
_eofLed1(false),
_eofLed2(false),
_timerReset(false),
_pThread(0)
{
	setupUi(parent);

	// create the main piraq execution thread.
	_pThread = new CP2ExecThread(_dspObjFile, _configFile);
	
	// start the thread
	_pThread->start();
	
	// set up the palettes
	_greenPalette = _chan0led->palette();
	_greenPalette.setColor(QPalette::Active, QPalette::Background, QColor("green"));
	_redPalette = _greenPalette;
	_redPalette.setColor(QPalette::Active, QPalette::Background, QColor("red"));

	// initializze the stats display
	timerEvent(0);

	// initialize eof leds to green
	_chan0led->setPalette(_greenPalette);
	_chan1led->setPalette(_greenPalette);
	_chan2led->setPalette(_greenPalette);

	// connect the close button to the closeSlot()
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(closeSlot()));

	// start the statistics timer
	_timerId = startTimer(1000);
}
/////////////////////////////////////////////////////////////////////
CP2Exec::~CP2Exec()
{
	_pThread->stop();
	_pThread->wait();
}

//////////////////////////////////////////////////////////////////////
void
CP2Exec::timerEvent(QTimerEvent*) 
{
	double rate1;
	double rate2;
	double rate3;
	int errors1;
	int errors2;
	int errors3;
	int pulses1;
	int pulses2;
	int pulses3;
	bool eof[3];

	double az[3];
	double el[3];
	unsigned int sweep[3];
	unsigned int volume[3];

	_pThread->antennaInfo(az, el, sweep, volume);
	_chan0Az->setText(QString("%1").arg(az[0],0,'f',1));
	_chan1Az->setText(QString("%1").arg(az[1],0,'f',1));
	_chan2Az->setText(QString("%1").arg(az[2],0,'f',1));

	_chan0El->setText(QString("%1").arg(el[0],0,'f',1));
	_chan1El->setText(QString("%1").arg(el[1],0,'f',1));
	_chan2El->setText(QString("%1").arg(el[2],0,'f',1));

	_chan0Sweep->setNum((long)sweep[0]);
	_chan1Sweep->setNum((long)sweep[1]);
	_chan2Sweep->setNum((long)sweep[2]);

	_chan0Volume->setNum((long)volume[0]);
	_chan1Volume->setNum((long)volume[1]);
	_chan2Volume->setNum((long)volume[2]);


	// fetch the statistics and the status 
	// from the piraq control thread.
	_pThread->rates(rate1, rate2, rate3);
	_pThread->pnErrors(errors1, errors2, errors3);
	_pThread->pulses(pulses1, pulses2, pulses3);
	_pThread->eof(eof);
	CP2ExecThread::STATUS status = _pThread->status();

	// set the error counters.
	_chan0errors->setNum(errors1);
	_chan1errors->setNum(errors2);
	_chan2errors->setNum(errors3);

	// set the rate displays
	_chan0pulseRate->setNum((int)rate1);
	_chan1pulseRate->setNum((int)rate2);
	_chan2pulseRate->setNum((int)rate3);

	// set the pulse counters
	_chan0pulseCount->setNum((int)(pulses1/1000));
	_chan1pulseCount->setNum((int)(pulses2/1000));
	_chan2pulseCount->setNum((int)(pulses3/1000));

	// set the eof leds.
	QColor c = QColor("red");
	if (eof[0]) {
		if (_eofLed0 != eof[0]) {
			_chan0led->setPalette(_redPalette);
			_eofLed0 = eof[0];
		}
	}
	if (eof[1]) {
		if (_eofLed1 != eof[1]) {
			_chan1led->setPalette(_redPalette);
			_eofLed1 = eof[1];
		}
	}
	if (eof[2]) {
		if (_eofLed2 != eof[2]) {
			_chan2led->setPalette(_redPalette);
			_eofLed2 = eof[2];
		}
	}

	switch (status) {
		case CP2ExecThread::RUNNING:
			_statusText->setText("Running");
			if (!_timerReset) {
				// now that we are running, reset the timer 
				// to the desired update interval
				//killTimer(_timerId);
				//startTimer(_statsUpdateInterval*1000);
				_timerReset = true;
			}
			break;
		case CP2ExecThread::STARTUP:
			_statusText->setText("Starting...");
			break;
		case CP2ExecThread::PIRAQINIT:
			_statusText->setText("Initializing Piraqs...");
			break;
	}


}
/////////////////////////////////////////////////////////////////////
