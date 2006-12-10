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
// Fft.hh
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// March 2006
//
///////////////////////////////////////////////////////////////

#ifndef Fft_hh
#define Fft_hh

#include <string>
#include <fftw3.h>
#include "Complex.hh"

using namespace std;

////////////////////////
// This class

class Fft {
  
public:
  
  // constructor - initializes for given size
  
  Fft (int n);
  
  // destructor
  
  ~Fft();

  // perform fwd fft

  void fwd(const Complex_t *in, Complex_t *out);

  // perform inverse fft

  void inv(const Complex_t *in, Complex_t *out);

protected:
  
private:

  int _n;
  double _sqrtN;
  fftw_plan _fftFwd;
  fftw_plan _fftBck;
  fftw_complex *_in;
  fftw_complex *_out;

};

#endif




