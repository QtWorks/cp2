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
///////////////////////////////////////////////////////////////
// Pulse.cc
//
// Mike Dixon, RAP, NCAR, P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// July 2003
//
////////////////////////////////////////////////////////////////

#include <iomanip>
#include <iostream>
#include <fstream>
#include <cerrno>
#include <cmath>
#include "Pulse.hh"
using namespace std;

// Constructor

Pulse::Pulse(const Params &params,
             int seqNum,
             int nGates,
             double time,
             double prt,
             double el,
             double az,
             bool isHoriz,
             const float *iqc,
             const float *iqx /* = NULL */) :
        _params(params),
        _nClients(0),
        _seqNum(seqNum),
        _nGates(nGates),
        _time(time),
        _prt(prt),
        _el(el),
        _az(az),
        _isHoriz(isHoriz)
  
{
  _prf = 1.0 / _prt;

  iqc = new float[nGates];
  memcpy(_iqc, iqc, nGates * sizeof(float));

  if (iqx == NULL) {
    _iqx = NULL;
  } else {
    iqx = new float[nGates];
    memcpy(_iqx, iqx, nGates * sizeof(float));
  }

}

// destructor

Pulse::~Pulse()

{
  if (_iqc) {
    delete[] _iqc;
  }
  if (_iqx) {
    delete[] _iqx;
  }
}

/////////////////////////////////////
// print

void Pulse::print(ostream &out) const
{

  out << "  nGates: " << _nGates << endl;
  out << "  seqNum: " << _seqNum << endl;
  out << "  time: " << _time << endl;
  out << "  prt: " << _prt << endl;
  out << "  prf: " << _prf << endl;
  out << "  el: " << _el << endl;
  out << "  az: " << _az << endl;
  out << "  isHoriz: " << _isHoriz << endl;
  out << "  have Xpol: " << (_iqx? "true" : "false") << endl;
  
}

/////////////////////////////////////////////////////////////////////////
// Memory management.
// This class uses the notion of clients to decide when to delete itself.
// When _nClients drops from 1 to 0, it will call delete on the this pointer.

int Pulse::addClient(const string &clientName)
  
{
  _nClients++;
  if (_params.debug >= Params::DEBUG_EXTRA_VERBOSE) {
    cerr << "Pulse add client, seqNum, nClients, az, client: "
	 << _seqNum << ", " << _nClients << ", " << _az << ", "
         << clientName << endl;
  }
  return _nClients;
}

int Pulse::removeClient(const string &clientName)

{
  _nClients--;
  if (_params.debug >= Params::DEBUG_EXTRA_VERBOSE) {
    cerr << "Pulse rem client, seqNum, nClients, az, client: "
	 << _seqNum << ", " << _nClients << ", " << _az << ", "
         << clientName << endl;
  }
  return _nClients;
}

