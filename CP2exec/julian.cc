//  This looks like C, but it's really C++!!
//      $Id: Julian.cc,v 1.2 2003/03/12 17:45:55 rich Exp $
//
//      Module:          Julian.cc
//      Original Author: Rich Neitzel
//      Copywrited by the National Center for Atmospheric Research
//      Date:            $Date: 2003/03/12 17:45:55 $
//
// revision history
// ----------------
// $Log: Julian.cc,v $
// Revision 1.2  2003/03/12 17:45:55  rich
// *** empty log message ***
//
// Revision 1.1.1.1  2001/06/28 14:46:02  thor
// Offical cvs version
//
// Revision 1.2  1999/10/27 15:56:00  thor
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
static char rcsid[] = "$Date: 2003/03/12 17:45:55 $ $RCSfile: Julian.cc,v $ $Revision: 1.2 $";

#include "Julian.hh"

#include <iostream>

using namespace RTFCommon;

static int months[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int Julian::day(int month, int day, int year)
{
  if (month > 11)
    return -1;

  int leap = isleap(year);

  int jday = day;

  if (jday > months[month])
    {
      if ((month == 1) && leap)
	{
	  if (jday > 29)
	    return -1;
	}
      else
	return -1;
    }

  if (month > 0)
    for (int i = 0; i < month; i++)
      jday += months[i];

  if (leap && (month > 1))
    jday++;

  return jday;
}

int Julian::crack(int jday, int *day, int *month, int leap)
{
  if (leap)
    {
      if (jday >= 40)
	--jday;
    }

  int c = 0;
  
  for (int i = 0; i < 12; i++)
    {
      int top = c + months[i];

      if (jday == top)
	{
	  *day = months[i];
	  *month = i;
	  return 0;
	}
      else if (jday < top)
	{
	  *day = jday - c;
	  *month = i;
	  return 0;
	}
      c = top;
    }
  return -1;
}
