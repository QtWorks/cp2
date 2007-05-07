#ifndef PCITIMERINC_
#define PCITIMERINC_

#include <vector>

typedef 
union HIMEDLO
{	
	unsigned int		himedlo;
	struct 
	{
		unsigned char		lo,med,hi,na;
	} byte;
} HIMEDLO;

typedef 
union HILO {	
	unsigned short	hilo;
	struct
	{
		unsigned char	lo,hi;
	} byte;
} HILO;

//typedef	struct TESTPULSE {
//	HILO		delay;
//	HILO		width;
//} TESTPULSE;

//typedef	struct TIMER {
//	SEQUENCE    seq[MAXSEQUENCE];
//	TESTPULSE   tp[8];
//	HILO        seqdelay,sync;
//	int         timingmode,seqlen;
//	float       clockfreq,reffreq,phasefreq;
//	char*       base;
//}	TIMER;
//typedef	struct SEQUENCE {
//	HILO		period;
//	int		pulseenable;
//	int		polarization;
//	int		phasedata;
//	int		index;
//} SEQUENCE;

/// A configuration structure for the PciTimer
class PciTimerConfig {
public:
	/// Constructor
	PciTimerConfig(int timingMode, float radarPrt, float xmitPulseWidth);
	/// Destructor
	virtual ~PciTimerConfig();
	/// Reset to default values (1 sequence, all counts set to 0)
	void reset();
void addSequence(int length, 
							unsigned char pulseMask,
							int polarization,
							int phase);
void setBpulse(int index, unsigned short width, unsigned short delay);
	/// There are six BPULSE signals, which can be triggered at the beginning of
	/// each sequence. However, a sequeence can be configured to not trigger a given
	/// BPULSE. Each BPULSE can have a width and a delay.
	struct Bpulse {
		HILO width;   ///< The BPULSE width in counts.
		HILO delay;   ///< The BPULSE delay in counts.
	} _bpulse[6];

	/// Up to six sequences can be defined. 
	struct Sequence {
		HILO length;       ///< The length of this sequence in counts.
		unsigned char bpulseMask;  ///< Enable a BPULSE by setting bits 0-5.
		int polarization; ///< Not sure what this is for.
		int phase;        ///< Not sure what this is for.
	};
	std::vector<Sequence> _sequences;
	/// Timing mode: 0 - internal prf generation, 1 - external prf generation
	int _timingMode;
	/// The PRT in seconds
	float radarPrt;
	/// The transmit pulse width in secs.
	float xmitPulseWidth
 };



/// The NCAR PCI based timer card.
class PciTimer {
public:
	PciTimer();
	~PciTimer();
	int timer_config();
int  init(TIMER *timer, int boardnumber);
void timer_reset(TIMER *timer);
void timer_set(TIMER *timer);
void timer_start(TIMER *timer);
void timer_stop(TIMER *timer);
int	 timer_test(TIMER *timer);

protected:
	/// The PCI address of the timer base register
	char* _base;
	/// The configuration
	PciTimerConfig _config;
	float _clockfreq;
	float _reffreq;
	float _phasefreq;


};

#endif
