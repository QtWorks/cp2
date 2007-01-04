#ifndef CP2PPIH
#define CP2PPIH
#include <winsock2.h>		//	no redefinition errors if before Qt includes?
#include <qsocketdevice.h> 
#include <qsocketnotifier.h>
#include <qevent.h>
#include <qbuttongroup.h>

#include <deque>
#include <set>
#include <map>
#include <string>

// request this much space for the socket receive buffer
#define CP2PPI_RCVBUF 25000000

// The base class created from the designer .ui specification
#include "CP2PPIBase.h"

// Coponents from the QtToolbox
#include <PPI/PPI.h>
#include <ColorBar/ColorMap.h>
#include <ColorBar/ColorBar.h>

// CP2 timeseries network transfer protocol.
#include "CP2Net.h"

// ProductInfo knows the characteristics of a particular 
// PPI product display
#include "PpiInfo.h"

// Clases used in the moments computatons:
#include "MomentsCompute.hh"
#include "MomentsMgr.hh"
#include "Params.hh"
#include "Pulse.hh"

#define PIRAQ3D_SCALE	1.0/(unsigned int)pow(2,31)	

// product types:
enum	PPITYPE {	
	S_DBMHC,	///< S-band dBm horizontal co-planar
	S_DBMVC,	///< S-band dBm vertical co-planar
	S_DBZHC,	///< S-band dBz horizontal co-planar
	S_DBZVC,	///< S-band dBz vertical co-planar
	S_SNR,		///< S-band SNR
	S_VEL,		///< S-band velocity
	S_WIDTH,	///< S-band spectral width
	S_RHOHV,	///< S-band rhohv
	S_PHIDP,	///< S-band phidp
	S_ZDR,		///< S-band zdr
	X_DBMHC,	///< X-band dBm horizontal co-planar
	X_DBMVX,	///< X-band dBm vertical cross-planar
	X_DBZHC,	///< X-band dBz horizontal co-planar
	X_SNR,		///< X-band SNR
	X_VEL,		///< X-band velocity
	X_WIDTH,	///< X-band spectral width
	X_LDR		///< X-band LDR
}; 


class CP2PPI : public CP2PPIBase {
	Q_OBJECT
public:
	CP2PPI();
	~CP2PPI();

public slots:
	// Call when data is available on the data socket.
	void newDataSlot(
		int socket         ///< File descriptor of the data socket
		);
	virtual void ppiTypeSlot(int ppiType);
	void tabChangeSlot(QWidget* w);
	void pauseSlot(bool flag);	//	start/stop process, display
    void zoomInSlot();

	void doSslot(bool);
	void doXslot(bool);
    void zoomOutSlot();
	void panSlot(int panIndex);
	// Call when data is available on the data socket.

protected:

	bool _processSband;
	bool _processXband;

	void initializeSocket(); 
	QSocketDevice*   _pSocket;
	QSocketNotifier* _pSocketNotifier;
	int				_dataGramPort;
	int				_prevPulseCount[3];			///<	prior cumulative pulse count, used for throughput calcs
	int				_pulseCount[3];				///<	cumulative pulse count
	int				_errorCount[3];				///<	cumulative error count
	bool			_eof[3];						///<    set true when fifo eof occurs. Used so that we don't
	long long       _lastPulseNum[3];
	///<    keep setting the fifo eof led.
	char*   _pSocketBuf;

	// how often to update the statistics (in seconds)
	int _statsUpdateInterval;

	PPITYPE        _ppiSType;
	// The builtin timer will be used to calculate beam statistics.
	void timerEvent(QTimerEvent*);

	/// Moment computation parameters for S band
	Params _Sparams;

	/// Moment computation parameters for X band
	Params _Xparams;

	/// The S band moment computation engine.  Pulses
	/// are passed to _momentsCompute. It will make a beam 
	/// available when enough pulses have been provided.
	MomentsCompute* _momentsSCompute;

	/// The X band moment computation engine.  Pulses
	/// are passed to _momentsCompute. It will make a beam 
	/// available when enough pulses have been provided.
	MomentsCompute* _momentsXCompute;

	QWidget* _ppiSwidget;

	QWidget* _ppiXwidget;

	double _azSband;

	double _azXband;

	/// Process the pulse, feeding it to the moments processor
	/// @param pPulse The pulse to be processed. 
	void processPulse(CP2Pulse* pPulse);

	/// The collator collects and matches time tags
	/// from H and V Xband channels
	CP2PulseCollator _collator;

	/// For each PPITYPE, there will be an entry in this map.
	std::map<PPITYPE, PpiInfo> _ppiInfo;

	/// This set contains PPITYPEs for all S band moments plots
	std::set<PPITYPE> _sMomentsList;

	/// This set contains PPITYPEs for all X band moments plots
	std::set<PPITYPE> _xMomentsList;

	/// save the button group for each tab,
	/// so that we can find the selected button
	/// and change the plot type when tabs are switched.
	std::vector<QButtonGroup*> _tabButtonGroups;

	/// initialize all of the book keeping structures
	/// for the various plots.
	void initPlots();

	/// add a tab to the plot type selection tab widget.
	/// Radio buttons are created for all of specified
	/// plty types, and grouped into one button group.
	/// _ppiInfo provides the label information for
	/// the radio buttons.
	/// @param tabName The title for the tab.
	/// @param types A set of the desired PPITYPE types 
	/// @return The button group that the inserted buttons
	/// belong to.
	QButtonGroup* addPlotTypeTab(std::string tabName, std::set<PPITYPE> types);

	/// Add a beam of S band products to the ppi
	void addSbeam(Beam* pBeam);

	/// Add a beam of X band products to the ppi
	void addXbeam(Beam* pBeam);

    // true to pause the display. Data will still be coming int,
	// but not sent to the display.
	bool _pause;
	
	ColorBar* _colorBar;
	int _gates;
	double _beamWidth;

	/// Set true when the Sband ppi is active,
	/// false when Xband is active.
	bool _ppiSactive;

	int _nVarsSband;
	std::vector<ColorMap*> _mapsSband;
	std::vector<std::vector<double> > _beamSData;

	int _nVarsXband;
	std::vector<ColorMap*> _mapsXband;
	std::vector<std::vector<double> > _beamXData;

	CP2Packet packet;


  };

#endif

