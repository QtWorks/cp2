#include "stdafx.h"

#include "time.h"
#include "Julian.h"

int get_julian_day(void)
{
        static Julian julian;

		time_t t = time(0);

		struct tm *tms = gmtime(&t);

		return julian.day(tms->tm_mon, tms->tm_mday, tms->tm_year);
}