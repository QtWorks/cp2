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
// GateSpectra.cc
//
// Mike Dixon, RAP, NCAR, P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// July 2005
//
////////////////////////////////////////////////////////////////
//
// Spectra for individual gates.
//
// Used to store computed spectra and other details so that these
// do not need to be recomputed for the clutter filtering step.
//
////////////////////////////////////////////////////////////////

#include <iostream>
#include "GateSpectra.hh"
using namespace std;

// Constructor

GateSpectra::GateSpectra(int nSamples) :
  _nSamples(nSamples)
  
{
  _trip1IsStrong = true;
  _censorOnPowerRatio = false;
  _prtSecs = 0.001;
  _powerStrong = 0.0;
  _velStrong = 0.0;
  _iq = NULL;
  _spectrumOrig = NULL;
  _spectrumStrongTrip = NULL;
  _magWeakTrip = NULL;
}

// destructor

GateSpectra::~GateSpectra()

{
  if (_iq) {
    delete[] _iq;
  }
  if (_spectrumOrig) {
    delete[] _spectrumOrig;
  }
  if (_spectrumStrongTrip) {
    delete[] _spectrumStrongTrip;
  }
  if (_magWeakTrip) {
    delete[] _magWeakTrip;
  }
}

//////////////////
// set spectra

void GateSpectra::setIq(const Complex_t *iq)

{
  if (_iq == NULL) {
    _iq = new Complex_t[_nSamples];
  }
  memcpy(_iq, iq, _nSamples * sizeof(Complex_t));
}

void GateSpectra::setSpectrumOrig(const Complex_t *spec)

{
  if (_spectrumOrig == NULL) {
    _spectrumOrig = new Complex_t[_nSamples];
  }
  memcpy(_spectrumOrig, spec, _nSamples * sizeof(Complex_t));
}

void GateSpectra::setSpectrumStrongTrip(const Complex_t *spec)

{
  if (_spectrumStrongTrip == NULL) {
    _spectrumStrongTrip = new Complex_t[_nSamples];
  }
  memcpy(_spectrumStrongTrip, spec, _nSamples * sizeof(Complex_t));
}

void GateSpectra::setMagWeakTrip(const double *mag)

{
  if (_magWeakTrip == NULL) {
    _magWeakTrip = new double[_nSamples];
  }
  memcpy(_magWeakTrip, mag, _nSamples * sizeof(double));
}

