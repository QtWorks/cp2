//  This looks like C, but it's really C++!!
//      $Id: Julian.hh,v 1.2 2003/03/12 17:45:55 rich Exp $
//
//      Module:          Julian.hh
//      Original Author: Rich Neitzel
//      Copywrited by the National Center for Atmospheric Research
//      Date:            $Date: 2003/03/12 17:45:55 $
//
// revision history
// ----------------
// $Log: Julian.hh,v $
// Revision 1.2  2003/03/12 17:45:55  rich
// *** empty log message ***
//
// Revision 1.1.1.1  2001/06/28 14:46:02  thor
// Offical cvs version
//
// Revision 1.2  1999/10/27 15:56:01  thor
// Fixed incorrect Sept. day count and added isleap().
//
// Revision 1.1.1.1  1999/10/13 19:04:12  thor
// Imported
//
// Revision 1.1  1998/06/19 18:40:30  thor
// Initial revision
//
//
//
// description:
//        
//
#ifndef _JULIAN_HH
#define _JULIAN_HH


  class Julian {
  public:
    Julian() {}

    int day(int month, int day, int year);

    int crack(int jday, int *day, int *month, int leap = 0);

    int isleap(int year)
    {
      if ((year % 4) != 0)
	return 0;
      else if ((year % 400) == 0)
	return 1;
      else if ((year % 100) == 0)
	return 0;
      else
	return 1;
    }
  };

#endif // _JULIAN_HH
