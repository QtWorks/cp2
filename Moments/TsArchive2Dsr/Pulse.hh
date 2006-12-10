/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 ** Copyright UCAR (c) 1992 - 1999
 ** University Corporation for Atmospheric Research(UCAR)
 ** National Center for Atmospheric Research(NCAR)
 ** Research Applications Program(RAP)
 ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
 ** All rights reserved. Licenced use only.
 ** Do not copy or distribute without authorization
 ** 1999/03/14 14:18:54
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
/////////////////////////////////////////////////////////////
// Pulse.hh
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// April 2006
//
///////////////////////////////////////////////////////////////

#ifndef Pulse_hh
#define Pulse_hh

#include <string>
#include <vector>
#include <deque>
#include <dataport/port_types.h>
#include "Params.hh"
#include "Complex.hh"

using namespace std;

////////////////////////
// This class

class Pulse {
  
public:

  Pulse(const Params &params,
        double fSyClkMhz);

  ~Pulse();
  
  // read pulse data
  // Returns 0 on success, -1 on failure

  int read(FILE *in);
  
  // compute phase differences between this pulse and previous ones
  // to be able to cohere to multiple trips
  //
  // Before this method is called, this pulse should be added to
  // the queue.
  
  int computePhaseDiffs(const deque<Pulse *> &pulseQueue,
			int maxTrips);

  // printing
  
  void print(ostream &out) const;

  // get methods

  int getNGates() const { return _nGates; }
  int getNChannels() const { return _nChannels; }
  long getSeqNum() const { return _iSeqNum; }
  double getFTime() const { return _ftime; }
  time_t getTime() const { return _time; }
  double getPrt() const { return _prt; }
  double getPrf() const { return _prf; }
  double getEl() const { return _el; }
  double getAz() const { return _az; }
  double getPhaseDiff0() const { return _phaseDiff[0]; }
  double getPhaseDiff1() const { return _phaseDiff[1]; }
  const fl32 *getIq() const { return _iq; }
  bool isHoriz() const; // is horizontally polarized

  // Memory management.
  // This class uses the notion of clients to decide when it should be deleted.
  // If removeClient() returns 0, the object should be deleted.
  
  int addClient(const string &clientName);
  int removeClient(const string &clientName);
  
protected:
private:

  const Params &_params;
  int _nClients;

  // IFD clock rate in MHZ

  double _fSyClkMhz;

  // RVP8 pulse header fields
  
  int _iVersion;
  int _iFlags;
  int _iMSecUTC;
  int _iTimeUTC;
  int _iBtimeAPI;
  int _iSysTime;
  int _iPrevPRT;
  int _iNextPRT;
  int _iSeqNum;
  int _iAqMode;
  int _iPolarBits;
  int _iTxPhase;
  int _iAz;
  int _iEl;
  int _iNumVecs;
  int _iMaxVecs;
  int _iVIQPerBin;
  int _iTgBank;
  int _iTgWave;

  int _uiqPerm[2];
  int _uiqOnce[2];

  double _fBurstMag[2];
  int _iBurstArg[2];
  
  // derived from pulse header

  int _nGates;
  int _nChannels;
  time_t _time;
  double _ftime;
  double _prt;
  double _prf;
  double _el;
  double _az;
  double _phaseDiff[2]; // phase difference from previous pulse
  vector<double> _phaseDiffs;

  // floating point IQ data

  fl32 *_iq;

  // functions
  
  int _readPulseHeader(FILE *in);
  void _deriveFromPulseHeader();
  int _readPulseData(FILE *in);
  
  void _vecFloatIQFromPackIQ
    (volatile fl32 fIQVals_a[], volatile const ui16 iCodes_a[],
     si32 iCount_a);

  void _subtract(const Complex_t &aa, const Complex_t &bb,
		 Complex_t &result, double &angle);
};

#endif

