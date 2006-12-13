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
#include "Params.hh"
#include "Complex.hh"

using namespace std;

////////////////////////
// This class

class Pulse {
  
public:

  // constructor

  // iqc: copolar IQ data, [i,q, i,q, i,q, ...]
  //      Must be set.
  // iqx: cross-opolar IQ data, [i,q, i,q, i,q, ...]
  //      Set to NULL if no cross-pol data

  Pulse(const Params &params,
        int seqNum,
        int nGates,
        double time,
        double prt,
        double el,
        double az,
        bool isHoriz,
        const float *iqc,
        const float *iqx = NULL);
  
  ~Pulse();
  
  // printing
  
  void print(ostream &out) const;

  // get methods

  int getNGates() const { return _nGates; }
  long getSeqNum() const { return _seqNum; }
  double getTime() const { return _time; } // secs in decimal
  double getPrt() const { return _prt; }
  double getPrf() const { return _prf; }
  double getEl() const { return _el; }
  double getAz() const { return _az; }
  bool isHoriz() const { return _isHoriz; } // is horizontally polarized?

  const float *getIqc() const { return _iqc; }
  const float *getIqx() const { return _iqx; }

  // Memory management.
  // This class uses the notion of clients to decide when it should be deleted.
  // If removeClient() returns 0, the object should be deleted.
  
  int addClient(const string &clientName);
  int removeClient(const string &clientName);
  
protected:
private:

  const Params &_params;

  int _nClients;

  int _seqNum;
  int _nGates;
  double _time;
  double _prt;
  double _prf;
  double _el;
  double _az;
  bool _isHoriz;
  
  // floating point IQ data
  // stored:
  //   (i,q,i,q,i,q,.... ) for iqc and iqx

  float *_iqc;
  float *_iqx;

};

#endif

