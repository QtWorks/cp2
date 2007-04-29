/*********************************************************************
 * Complex.cc
 *
 * Complex arithmetic
 *
 * Mike Dixon, RAP, NCAR, Boulder CO
 *
 * April 2007
 *
 *********************************************************************/

#include "Complex.hh"
#include <cmath>

#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.29577951308092
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.01745329251994372
#endif

using namespace std;

// all methods static

// compute complex product

Complex_t Complex::complexProduct(const Complex_t &c1,
                                  const Complex_t &c2)
  
{
  Complex_t product;
  product.re = (c1.re * c2.re) - (c1.im * c2.im);
  product.im = (c1.im * c2.re) + (c1.re * c2.im);
  return product;
}

// compute mean complex product of series

Complex_t Complex::meanComplexProduct(const Complex_t *c1,
                                      const Complex_t *c2,
                                      int len)
  
{
  
  double sumRe = 0.0;
  double sumIm = 0.0;
  
  for (int ipos = 0; ipos < len; ipos++, c1++, c2++) {
    sumRe += ((c1->re * c2->re) - (c1->im * c2->im));
    sumIm += ((c1->im * c2->re) + (c1->re * c2->im));
  }

  Complex_t meanProduct;
  meanProduct.re = sumRe / len;
  meanProduct.im = sumIm / len;

  return meanProduct;

}

// compute conjugate product

Complex_t Complex::conjugateProduct(const Complex_t &c1,
                                    const Complex_t &c2)
  
{
  Complex_t product;
  product.re = (c1.re * c2.re) + (c1.im * c2.im);
  product.im = (c1.im * c2.re) - (c1.re * c2.im);
  return product;
}
  
// compute mean conjugate product of series

Complex_t Complex::meanConjugateProduct(const Complex_t *c1,
                                        const Complex_t *c2,
                                        int len)
  
{
  
  double sumRe = 0.0;
  double sumIm = 0.0;

  for (int ipos = 0; ipos < len; ipos++, c1++, c2++) {
    sumRe += ((c1->re * c2->re) + (c1->im * c2->im));
    sumIm += ((c1->im * c2->re) - (c1->re * c2->im));
  }

  Complex_t meanProduct;
  meanProduct.re = sumRe / len;
  meanProduct.im = sumIm / len;

  return meanProduct;

}

// compute sum

Complex_t Complex::complexSum(const Complex_t &c1,
                              const Complex_t &c2)
  
{
  Complex_t sum;
  sum.re = c1.re + c2.re;
  sum.im = c1.im + c2.im;
  return sum;
}
 
/////////////////////////////////////////
// mean of complex values

Complex_t Complex::complexMean(Complex_t c1, Complex_t c2)

{
  Complex_t mean;
  mean.re = (c1.re + c2.re) / 2.0;
  mean.im = (c1.im + c2.im) / 2.0;
  return mean;
}

 
// compute magnitude

double Complex::mag(const Complex_t &cc)
  
{
  return sqrt(cc.re * cc.re + cc.im * cc.im);
}
  
// compute arg in degrees

double Complex::argDeg(const Complex_t &cc)
  
{
  double arg = 0.0;
  if (cc.re != 0.0 || cc.im != 0.0) {
    arg = atan2(cc.im, cc.re);
  }
  arg *= RAD_TO_DEG;
  return arg;
}

// compute arg in radians

double Complex::argRad(const Complex_t &cc)
  
{
  double arg = 0.0;
  if (cc.re != 0.0 || cc.im != 0.0) {
    arg = atan2(cc.im, cc.re);
  }
  return arg;
}

// compute difference between two angles
// in degrees

double Complex::diffDeg(double deg1, double deg2)
  
{
  
  double rad1 = deg1 * DEG_TO_RAD;
  double rad2 = deg2 * DEG_TO_RAD;
  
  Complex_t ang1, ang2;
  ang1.re = cos(rad1);
  ang1.im = sin(rad1);
  ang2.re = cos(rad2);
  ang2.im = sin(rad2);
  
  Complex_t conjProd = conjugateProduct(ang1, ang2);
  return argDeg(conjProd);
  
}

// compute difference between two angles
// in radians

double Complex::diffRad(double rad1, double rad2)
  
{
  
  Complex_t ang1, ang2;
  ang1.re = cos(rad1);
  ang1.im = sin(rad1);
  ang2.re = cos(rad2);
  ang2.im = sin(rad2);
  
  Complex_t conjProd = conjugateProduct(ang1, ang2);
  return argRad(conjProd);
  
}

// compute sum of two angles in degrees

double Complex::sumDeg(double deg1, double deg2)
  
{
  
  double rad1 = deg1 * DEG_TO_RAD;
  double rad2 = deg2 * DEG_TO_RAD;
  
  Complex_t ang1, ang2;
  ang1.re = cos(rad1);
  ang1.im = sin(rad1);
  ang2.re = cos(rad2);
  ang2.im = sin(rad2);
  
  Complex_t compProd = complexProduct(ang1, ang2);
  double sum = argDeg(compProd);
  return sum;
  
}

// compute sum between two angles
// in radians

double Complex::sumRad(double rad1, double rad2)
  
{
  
  Complex_t ang1, ang2;
  ang1.re = cos(rad1);
  ang1.im = sin(rad1);
  ang2.re = cos(rad2);
  ang2.im = sin(rad2);
  
  Complex_t compProd = complexProduct(ang1, ang2);
  double sum = argRad(compProd);
  return sum;
  
}

// compute mean of two angles in degrees

double Complex::meanDeg(double deg1, double deg2)
  
{

  double diff = diffDeg(deg2, deg1);
  double mean = sumDeg(deg1, diff / 2.0);

  return mean;

}

// compute mean of two angles in radians

double Complex::meanRad(double rad1, double rad2)
  
{

  double diff = diffRad(rad2, rad1);
  double mean = sumRad(rad1, diff / 2.0);
  return mean;

}

//////////////////////////////////////
// compute power

double Complex::power(const Complex_t &cc)
  
{
  return cc.re * cc.re + cc.im * cc.im;
}

/////////////////////////////////////////////
// compute mean power of series

double Complex::meanPower(const Complex_t *c1, int len)
  
{
  double sum = 0.0;
  for (int ipos = 0; ipos < len; ipos++, c1++) {
    sum += ((c1->re * c1->re) + (c1->im * c1->im));
  }
  return sum / len;
}

