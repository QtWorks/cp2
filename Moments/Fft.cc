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
// Fft.cc
//
// Mike Dixon, RAP, NCAR, P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// March 2006
//
///////////////////////////////////////////////////////////////
//
// Fft handles Fast Fourier Transform computations
//
////////////////////////////////////////////////////////////////

#include "Fft.hh"
#include <iostream>
#include <cmath>
using namespace std;

// Constructor

Fft::Fft(int n) :
  _n(n)
  
{

  _sqrtN = sqrt((double) _n);
  
  // set up Fft plans

  _in = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * _n);
  _out = (fftw_complex *) fftw_malloc(sizeof(fftw_complex) * _n);

  _fftFwd = fftw_plan_dft_1d(_n, _in, _out, FFTW_FORWARD, FFTW_MEASURE);
  _fftBck = fftw_plan_dft_1d(_n, _in, _out, FFTW_BACKWARD, FFTW_MEASURE);

}

// destructor

Fft::~Fft()

{

  fftw_destroy_plan(_fftFwd);
  fftw_destroy_plan(_fftBck);
  
  fftw_free(_in);
  fftw_free(_out);

}


///////////////////////////////////////////////
// compute forward

void Fft::fwd(const Complex_t *in, Complex_t *out)
  
{

  memcpy(_in, in, _n * sizeof(Complex_t));
  fftw_execute(_fftFwd);

  // adjust by sqrt(n)

  double *oo = (double *) _out;
  for (int ii = 0; ii < _n; ii++, out++) {
    out->re = *oo / _sqrtN;
    oo++;
    out->im = *oo / _sqrtN;
    oo++;
  }

}

///////////////////////////////////////////////
// compute inverse

void Fft::inv(const Complex_t *in, Complex_t *out)
  
{
  
  memcpy(_in, in, _n * sizeof(Complex_t));
  fftw_execute(_fftBck);

  // adjust by sqrt(n)

  double *oo = (double *) _out;
  for (int ii = 0; ii < _n; ii++, out++) {
    out->re = *oo / _sqrtN;
    oo++;
    out->im = *oo / _sqrtN;
    oo++;
  }

}

