/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 ** Copyright UCAR (c) 1992 - 1997
 ** University Corporation for Atmospheric Research(UCAR)
 ** National Center for Atmospheric Research(NCAR)
 ** Research Applications Program(RAP)
 ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
 ** All rights reserved. Licenced use only.
 ** Do not copy or distribute without authorization
 ** 1997/9/26 14:18:54
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
////////////////////////////////////////////////////////////////////
// TaArray.hh
//
// Array wrapper
//
// Mike Dixon, RAL, NCAR, POBox 3000, Boulder, CO, 80307, USA
//
// Dec 2006
//
////////////////////////////////////////////////////////////////////
//
// TaArray allows you to declare an object, and allocate an array
// in that object, and have the array be automatically freed when
// the destructor is called.
//
// If you make the object an automatic variable, the destructor
// will be called when the object goes out of scope.
//
////////////////////////////////////////////////////////////////////

#ifndef TA_ARRAY_HH
#define TA_ARRAY_HH

using namespace std;

template <class T>
class TaArray
{
public:

  TaArray();
  ~TaArray();

  // Alloc array

  T *alloc(int nelem);

  // free array

  void free();

private:

  T *_buf;
  int _nelem;

};

// The Implementation.

template <class T>
TaArray<T>::TaArray()
{
  _buf = NULL;
  _nelem = 0;
}

template <class T>
TaArray<T>::~TaArray()
{
  free();
}

template <class T>
T *TaArray<T>::alloc(int nelem)
{
  free();
  _buf = new T[nelem];
  _nelem = nelem;
  return _buf;
}

template <class T>
void TaArray<T>::free()
{
  if (_buf != NULL) {
    delete[] _buf;
  }
  _buf = NULL;
  _nelem = 0;
}

#endif
