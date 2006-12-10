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
// GateSpectra.hh
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// May 2005
//
///////////////////////////////////////////////////////////////
//
// Spectra for individual gates.
//
// Used to store computed spectra and other details so that these
// do not need to be recomputed for the clutter filtering step.
//
////////////////////////////////////////////////////////////////

#ifndef GateSpectra_hh
#define GateSpectra_hh

#include <string>
#include <vector>
#include "Complex.hh"

using namespace std;

class GateSpectra {
  
public:

  GateSpectra(int nSamples);
  ~GateSpectra();

  void setTrip1IsStrong(bool state) { _trip1IsStrong = state; }
  void setCensorOnPowerRatio(bool state) { _censorOnPowerRatio = state; }
  void setPrtSecs(double secs) { _prtSecs = secs; }
  void setPowerStrong(double power) { _powerStrong = power; }
  void setVelStrong(double vel) { _velStrong = vel; }
  void setIq(const Complex_t *iq);
  void setSpectrumOrig(const Complex_t *spec);
  void setSpectrumStrongTrip(const Complex_t *spec);
  void setMagWeakTrip(const double *mag);

  bool getTrip1IsStrong() const { return _trip1IsStrong; }
  bool getCensorOnPowerRatio() const { return _censorOnPowerRatio; }
  double getPrtSecs() const { return _prtSecs; }
  double getPowerStrong() const { return _powerStrong; }
  double getVelStrong() const { return _velStrong; }
  const Complex_t *getIq() const { return _iq; }
  const Complex_t *getSpectrumOrig() const { return _spectrumOrig; }
  const Complex_t *getSpectrumStrongTrip() const { return _spectrumStrongTrip; }
  const double *getMagWeakTrip() const { return _magWeakTrip; }

protected:
private:

  int _nSamples;

  bool _trip1IsStrong;
  bool _censorOnPowerRatio;

  double _prtSecs;
  double _powerStrong;
  double _velStrong;

  Complex_t *_iq;
  Complex_t *_spectrumOrig;
  Complex_t *_spectrumStrongTrip;
  double *_magWeakTrip;
  

};

#endif

