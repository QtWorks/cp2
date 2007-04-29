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
// Complex.hh
//
// Mike Dixon, RAP, NCAR
// P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// March 2006
//
///////////////////////////////////////////////////////////////

#ifndef Complex_hh
#define Complex_hh

typedef struct {
  double re;
  double im;
} Complex_t;

class Complex {

public:

  // all methods static
  
  // compute complex product
  
  static Complex_t complexProduct(const Complex_t &c1,
                                  const Complex_t &c2);
  
  static Complex_t meanComplexProduct(const Complex_t *c1,
                                      const Complex_t *c2,
                                      int len);
  // compute conjugate product
  
  static Complex_t conjugateProduct(const Complex_t &c1,
                                    const Complex_t &c2);
  
  // compute mean conjugate product of series
  
  static Complex_t meanConjugateProduct(const Complex_t *c1,
                                        const Complex_t *c2,
                                        int len);
  
  // compute sum

  static Complex_t complexSum(const Complex_t &c1,
                              const Complex_t &c2);
  
  // mean of complex values
  
  static Complex_t complexMean(Complex_t c1, Complex_t c2);

  // compute magnitude

  static double mag(const Complex_t &cc);
  
  // compute arg in degrees

  static double argDeg(const Complex_t &cc);
  

  // compute arg in radians

  static double argRad(const Complex_t &cc);
  

  // compute difference between two angles
  // in degrees

  static double diffDeg(double deg1, double deg2);
  
  // compute difference between two angles
  // in radians

  static double diffRad(double rad1, double rad2);
  
  // compute sum of two angles in degrees

  static double sumDeg(double deg1, double deg2);
  
  // compute sum between two angles
  // in radians

  static double sumRad(double rad1, double rad2);
  
  // compute mean of two angles in degrees

  static double meanDeg(double deg1, double deg2);
  
  // compute mean of two angles in radians

  static double meanRad(double rad1, double rad2);

  // compute power

  static double power(const Complex_t &cc);

  // compute mean power of series

  static double meanPower(const Complex_t *c1, int len);
  
};

#endif

