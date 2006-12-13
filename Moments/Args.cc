/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
 ** Copyright UCAR (c) 1992 - 1999
 ** University Corporation for Atmospheric Research(UCAR)
 ** National Center for Atmospheric Research(NCAR)
 ** Research Applications Program(RAP)
 ** P.O.Box 3000, Boulder, Colorado, 80307-3000, USA
 ** All rights reserved. Licenced use only.
 ** Do not copy or distribute without authorization
 ** 1999/03/14 13:58:59
 *=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
//////////////////////////////////////////////////////////
// Args.cc : command line args
//
// Mike Dixon, RAP, NCAR, P.O.Box 3000, Boulder, CO, 80307-3000, USA
//
// May 2006
//
//////////////////////////////////////////////////////////

#include "Args.hh"
#include <cstring>
using namespace std;

// constructor

Args::Args()

{
}

// destructor

Args::~Args()

{
}

// parse

int Args::parse(int argc, char **argv, Params &params)

{

  int iret = 0;

  // loop through args
  
  for (int i =  1; i < argc; i++) {

    if (!strcmp(argv[i], "--") ||
	!strcmp(argv[i], "-h") ||
	!strcmp(argv[i], "-help") ||
	!strcmp(argv[i], "-man")) {
      
      usage(cout);
      exit (0);
      
    } else if (!strcmp(argv[i], "-d") ||!strcmp(argv[i], "-debug")) {
      
      params.debug = Params::DEBUG_NORM;
      
    } else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "-verbose")) {
      
      params.debug = Params::DEBUG_VERBOSE;
      
    } else if (!strcmp(argv[i], "-v2") || !strcmp(argv[i], "-extra")) {
      
      params.debug = Params::DEBUG_EXTRA_VERBOSE;
      
    } // if
    
  } // i
  
  if (iret) {
    usage(cerr);
  }

  return (iret);
    
}

void Args::usage(ostream &out)
{

  out << "Usage: MomentsCompute [options as below]\n"
      << "options:\n"
      << "       [ --, -h, -help, -man ] produce this list.\n"
      << "       [ -d, -debug ] print debug messages\n"
      << "       [ -v, -verbose ] print verbose debug messages\n"
      << "       [ -v2, -extra ] print extra verbose debug messages\n"
      << endl;

}
