#ifndef CP2PPIH
#define CP2PPIH
#include "CP2PPIBase.h"
//	move to CP2.h:
#include <winsock2.h>		//	no redefinition errors if before Qt includes?
#include <qsocketdevice.h> 
#include <qsocketnotifier.h>
#include <qevent.h>
#include "../include/CP2.h"			//	CP2-wide Definitions
#include "../include/piraqx.h"		//	CP2-piraqx Definitions
#include "../include/dd_types.h"	//	CP2-piraqx data types

#include <vector>
#include <qtimer.h>
#include <ColorBar/ColorMap.h>
#include <ColorBar/ColorBar.h>

enum	{	XBAND_DATA_SOURCE,	SBAND_DATA_SOURCE,	DATA_SOURCES	};	//	data sources for display

//	definition of ColorBar min, max and button label for product to display 
struct productDisplayConfiguration {
	float rangeMin; 
	float rangeMax;
	char * label;
//	char * labelPrecision;	//	NCP needs x.xx display 
};

class CP2PPI: public CP2PPIBase
  {
  Q_OBJECT
  public:
	CP2PPI( int nVars, QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    virtual ~CP2PPI();
    
	//	add these as they are supported in UI 
//    void changeDir();
//    void clearVarSlot(int);
//    void panSlot(int);
	//	add these:
	void initializeSocket(); 
	void terminateSocket(); 
	void connectDataRcv();
	void ChangeDatagramPort(int);
	int	getParameters( CP2_PIRAQ_DATA_TYPE[] );		//	get program operating parameters from piraqx (or other) "housekeeping" header structure
//	void setupTestProducts( void );	//	set up test product buttons, maps, etc.
void setupTestProducts( productDisplayConfiguration * );	//	

public slots:
	void startStopDisplaySlot();	//	start/stop process, display
	void dataSourceSlot();	//	data source radar/internally-generated test 
    void addBeam();
    void productSelectSlot(int);
    void zoomInSlot();
    void zoomOutSlot();
	// Call when data is available on the data socket.
	void dataSocketActivatedSlot(int socket);	// File descriptor of the data socket 

protected:
	char			m_statusLogBuf[1024];		//	global status string for Status Log
	int				m_dataSource;		//	data source for PPI display: radar or test 
	//	network-related: 
	int				m_receiving;		//	receive-data state
	int				m_receivePacketCount;		//	cumulative receive packet count  
	QSocketDevice*   m_pDataSocket;
	QSocketNotifier* m_pDataSocketNotifier;
	CP2_PIRAQ_DATA_TYPE*   m_pSocketBuf;
	CP2_PIRAQ_DATA_TYPE* SABP;	//	S-band ABP data packet generated from VHVH alternating pulses
	int				m_dataGramPort;
	int				m_datagramPortBase;	//	
	int				m_dataChannel;		//	board source of data CP2 PIRAQ 1-3 
	int				m_packetCount;		//	cumulative packet count on current socket 

	int				_parametersSet;	//	set when piraqx parameters successfully initialized from received data
	//	parameters from piraqx (or other header) "housekeeping" for use by program: data types per piraqx.h
	uint4			_gates;
	uint4			_hits;
	uint4			_dataformat;	//	timeseries or 1 of 3 ABP types
    float4			_prt;			//	pulse repetition time in seconds
	//	parameters from piraqx for power calculations
	float4			_data_sys_sat;	//	receiver saturation power in dBm
	float4			_receiver_gain;		//	horizontal receiver gain
	float4			_vreceiver_gain;	//	vertical receiver gain
	float4			_noise_power;	//	noise power horizontal and vertical channels
	float4			_vnoise_power;	
	//	parameters from piraqx for velocity calculations
	float4			_frequency;		//	radar transmit frequency
	float4			_phaseoffset;	//	phase offset between V and H channels
	//	parameters from piraqx for reflectivity calculations
	float4			_rconst;		//	configured radar constant
	float4			_xmit_pulsewidth;	//	 
	float4			_rcvr_pulsewidth;	//	 
	float4			_peak_power;	//	  
	float4			_xmit_power;	//	  
	float4			_vxmit_power;	//	  
	float4			_antenna_gain;	//	  
	float4			_vantenna_gain;	//	  
	float4			_zdr_fudge_factor;	//	  
	float4			_zdr_bias;	//	 
	float4			_noise_phase_offset;	//	offset to provide noise immunity in velocity estimation 
	//	radar constants for H,V computed at runtime
	float4			_v_rconst;		//	computed V radar constant
	float4			_h_rconst;		//	computed H radar constant
	//	operating parameters derived from piraqx and N-hit implementation:	
	unsigned int	_PNOffset;		//	offset in bytes to pulsenumber from begin of CP2exec-generated packet
	unsigned int	_IQdataOffset;	//	offset to begin of data in (piraqx) packet
	int				_pulseStride;	//	length in bytes of 1 pulse: header + data
	int				_Nhits;			//	

//	CM stuff; retain whats necessary 
    double _angle;
    double _angleInc;
    int _nVars;
//    int _gates;
	double _beamWidth;
    std::vector<int> _varIndices;
	std::vector<std::vector<double> > _beamData;
	std::vector<ColorMap*> _maps;
	ColorBar* _colorBar;
    
    QTimer _timer;

  };

#endif

