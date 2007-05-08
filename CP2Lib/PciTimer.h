#ifndef PCITIMERINC_
#define PCITIMERINC_

#include <vector>

#define	PCITIMER_DEVICE_ID 	0x8504
#define	PCITIMER_VENDOR_ID	0x10E8   
#define	COUNTFREQ	6000000

/// PciTimer reset command, which actually calls for a reconfiguration.
/// Place the command code in location 0x3E of the dual ported ram.
#define	TIMER_RESET	0
/// PciTimer stop command.
/// Place the command code in location 0x3E of the dual ported ram.
#define	TIMER_STOP	1
/// PciTimer start command.
/// Place the command code in location 0x3E of the dual ported ram.
#define	TIMER_START	2

/// PciTimer service request. Placing this in location
/// 0x3F in the timer dual ported ram will cause it to
/// execute the command found in location 0x3E.
#define	TIMER_RQST	1

/// A maximum of 38 sequences can be specified. This is because
/// the first 0xC0 bytes in the dual ported ram are used for
/// sequence definitions, and each definitions uses 5 bytes.
#define	MAXSEQUENCE  38


/////////////////////////////////////////////////////////////////////////

/// The NCAR PCI based timer card is managed by this class. The timer is
/// reset and intialized during construction. The card may be started and 
/// stopped after that.
///
/// The PCI timer card has an onboard phase locked loop, which provides the 
/// basic clock for the pulse delay and pulse width counts.
///
/// The timer generates a basic PRT timing pulse train, which is output from
/// the card on the SYNC/TRIG_OUT line. The width of the PRT pulse is not
/// configurable.
///
/// The PRT pulse train can consist of aribtrary time deltas between pulses. Each
/// succesive delta is known as a sequence. Up to 38 sequences may be defined.
///
/// Additionally, 6 BPULSE signals can be triggered on each pulse of the PRT
/// train. A BPULSE has a delay and a width. Individual sequences can be 
/// configured to trigger zero or more of the BPULSE signals.
///
/// To use PciTimer:
/// <ul>
/// <li>Create a PciTimerConfig
/// <li>Add sequences via PciTimerConfig::addSequence() and/or PciTimerConfig::addPrt().
/// <li>Configure the BPULSE signals via PciTimerConfig::setBpulse(), 
/// <li>Create a PciTimer, passing the PciTimerConfig to the constructor.
/// <li>Call PciTimer::start() to start the timer.
/// <li>Call PciTimer::stop() to stop the timer.
/// <li>The timer will be stopped and reset if the PciTimer destructor is called.
/// </ul>
/// @todo Since the API is so sparse, PciTimerConfig can be merged into PciTimer.
class PciTimer {
	/// Used to arrange byte access to 3 byte integers
	typedef union TIMERHIMEDLO
	{	
		unsigned int himedlo;
		struct {
			unsigned char lo, med, hi, na;
		} byte;
	} TIMERHIMEDLO;

	/// Used to arrange byte access to 2 byte integers.
	typedef union TIMERHILO {	
		unsigned short hilo;
		struct {
			unsigned char lo, hi;
		} byte;
	} TIMERHILO;

public:
	/// Reset, confgure and intialize the timer card during construction.
	PciTimer(double systemClock, ///< The system clock, in Hz.
		int timingMode           ///< The timimg mode. 0 - generate the PRF onboard, 1 - the PRF comes from an external trigger
		);
	/// Destructor.
	~PciTimer();
	/// Add a new sequence step to the end of the sequence list.
	void addSequence(int length, ///< The length of this sequence, in counts
		unsigned char pulseMask, ///< A mask which defines which BPULSE signals are enabled during this sequence step.
		int polarization,        ///< Polarization value for this sequence step. Not clear what it does.
		int phase                ///< Phase value for this sequence step. Not sure what it does.
		);
	/// Add a new sequence step to the end of the sequence list.
	void addPrt(float prt,       ///< The length of this sequence, seconds.
		unsigned char pulseMask, ///< A mask which defines which BPULSE signals are enabled during this sequence step.
		int polarization,        ///< Polarization value for this sequence step. Not clear what it does.
		int phase                ///< Phase value for this sequence step. Not sure what it does.
		);
	/// Define a BPULSE, which is a pulse of a fixed delay and length.
	void setBpulse(int index, unsigned short width, unsigned short delay);

	/// Stop the timer,and reset it.
	void reset();
	/// Start the timer
	void start();
	/// Stop the timer
	void stop();
	/// return the current error flag.
	bool error();

protected:
	/// Place the timer configuration in the timer dual ported
	/// ram and tell the timer to use it.
	void configure();
	/// Handshake with the timer.
	int	 test();
	/// There are six BPULSE signals, which can be triggered at the beginning of
	/// each sequence. However, a sequeence can selectively choose which BPULSES to
	/// trigger. Each BPULSE can have a width and a delay.
	struct Bpulse {
		TIMERHILO width;   ///< The BPULSE width in counts.
		TIMERHILO delay;   ///< The BPULSE delay in counts.
	} _bpulse[6];
	/// Specify the parameters for a sequence. 
	struct Sequence {
		TIMERHILO length;       ///< The length of this sequence in counts.
		unsigned char bpulseMask;  ///< Enable a BPULSE by setting bits 0-5.
		int polarization; ///< Not sure what this is for.
		int phase;        ///< Not sure what this is for.
	};
	/// Collect all sequences. These sequences will be executed
	/// in the order that they are defined.
	std::vector<Sequence> _sequences;
	/// The system clock rate, in Hz. It is used in various timng calculations.
	double _systemClock;
	/// The timing mode: 0 - internal prf generation, 1 - external prf generation.
	int _timingMode;
	/// The PCI address of the timer base register
	char* _base;
	/// The clock frequency, in Hz.
	float _clockfreq;
	/// The reference clock frequency, in Hz.
	float _reffreq;
	/// The phase frequency. in Hz.
	float _phasefreq;
	// The sync configuration word. What does this do?
	TIMERHILO _sync;
	/// The seq delay. What does this do?
	TIMERHILO _seqdelay;
	/// Set true if there has been a timer error
	bool _error;


};

#endif
