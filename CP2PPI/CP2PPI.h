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
#define CP2PPI_RCVBUF 100000000

// The base class created from the designer .ui specification
#include "CP2PPIBase.h"

// Components from the QtToolbox
#include <PPI/PPI.h>
#include <ColorBar/ColorMap.h>
#include <ColorBar/ColorBar.h>

// CP2 timeseries network transfer protocol.
#include "CP2Net.h"

// ProductInfo knows the characteristics of a particular 
// PPI product display
#include "PpiInfo.h"

#define PIRAQ3D_SCALE	1.0/(unsigned int)pow(2,31)	

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

protected:
	// intialize the socket that the product data 
	// is received on
	void initializeSocket(); 
	/// Process the products as they come in. 
	/// Sband and X band products with identical 
	/// beam numbers are collected until a full
	/// set of products are received. The full set
	/// is then sent to the display.
	/// If a complete set is not collected for a given
	/// beam number, it will not be displayed.
	void processProduct(CP2Product* pProduct);
	/// The incoming product socket.
	QSocketDevice*   _pSocket;
	/// The notifier for the data socket; it is connected
	/// to new data slot.
	QSocketNotifier* _pSocketNotifier;
	/// The buffer that the product data will be read into
	/// from the socket.
	char*   _pSocketBuf;
	/// Incoming produt port number.
	int	_dataGramPort;
	int	_pulseCount[3];				
	// how often to update the statistics (in seconds)
	int _statsUpdateInterval;
	/// the currently selected display type
	PRODUCT_TYPES _ppiSType;
	// The builtin timer will be used to calculate beam statistics.
	void timerEvent(QTimerEvent*);
	/// For each PRODUCT_TYPES, there will be an entry in this map.
	std::map<PRODUCT_TYPES, PpiInfo> _ppiInfo;
	/// This set contains PRODUCT_TYPESs for all desired S band moments plots
	std::set<PRODUCT_TYPES> _sMomentsList;
	/// This set contains the list of all received S band products 
	/// that are on the desired list, and have the same time tag.
	/// When the length reaches the same size as _sMomentsList, 
	/// then we have all products for a given S band beam
	std::set<PRODUCT_TYPES> _currentSproducts;
	/// This set contains the list of all received X band products 
	/// that are on the desired list, and have the same time tag.
	/// When the length reaches the same size as _xMomentsList, 
	/// then we have all products for a given X band beam
	std::set<PRODUCT_TYPES> _currentXproducts;
	/// The current beam number for S band products, used
	/// for collating S band products as they are received.
	long long _currentSbeamNum;
	/// The current beam number for X band products, used
	/// for collating X band products as they are received.
	long long _currentXbeamNum;
	/// This set contains PRODUCT_TYPESs for all desired X band moments plots
	std::set<PRODUCT_TYPES> _xMomentsList;
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
	/// @param types A set of the desired PRODUCT_TYPES types 
	/// @return The button group that the inserted buttons
	/// belong to.
	QButtonGroup* addPlotTypeTab(std::string tabName, std::set<PRODUCT_TYPES> types);
	/// Add a beam of S band products to the ppi
	void displaySbeam(double az, double el);
	/// Add a beam of X band products to the ppi
	void displayXbeam(double az, double el);
    // true to pause the display. Data will still be coming in,
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
	std::vector<std::vector<double> > _beamSdata;

	int _nVarsXband;
	std::vector<ColorMap*> _mapsXband;
	std::vector<std::vector<double> > _beamXdata;

	CP2Packet packet;


  };

#endif

