#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>

#include "modem-time.h"

// ------------------------------------------------------
// program define
// ------------------------------------------------------

#

// ------------------------------------------------------
// julian time util..
// ------------------------------------------------------
typedef  unsigned short     uint16;
typedef  unsigned int     	uint32;

typedef struct
{
	/* Year [1980..2100] */
	uint16                          year;

	/* Month of year [1..12] */
	uint16                          month;

	/* Day of month [1..31] */
	uint16                          day;

	/* Hour of day [0..23] */
	uint16                          hour;

	/* Minute of hour [0..59] */
	uint16                          minute;

	/* Second of minute [0..59] */
	uint16                          second;

	/* Day of the week [0..6] [Monday .. Sunday] */
	uint16                          day_of_week;
}
time_julian_type;



#define TIME_JUL_OFFSET_S         432000UL
#define TIME_JUL_BASE_YEAR        1980
#define TIME_JUL_QUAD_YEAR        (366+(3*365))


static const uint16 year_tab[] = {
	0,                              /* Year 0 (leap year) */
	366,                            /* Year 1             */
	366+365,                        /* Year 2             */
	366+365+365,                    /* Year 3             */
	366+365+365+365                 /* Bracket year       */
};

/* The norm_month_tab table holds the number of cumulative days that have
      elapsed as of the end of each month during a non-leap year. */

static const uint16 norm_month_tab[] = {
	0,                                    /* --- */
	31,                                   /* Jan */
	31+28,                                /* Feb */
	31+28+31,                             /* Mar */
	31+28+31+30,                          /* Apr */
	31+28+31+30+31,                       /* May */
	31+28+31+30+31+30,                    /* Jun */
	31+28+31+30+31+30+31,                 /* Jul */
	31+28+31+30+31+30+31+31,              /* Aug */
	31+28+31+30+31+30+31+31+30,           /* Sep */
	31+28+31+30+31+30+31+31+30+31,        /* Oct */
	31+28+31+30+31+30+31+31+30+31+30,     /* Nov */
	31+28+31+30+31+30+31+31+30+31+30+31   /* Dec */
};

static const uint16 leap_month_tab[] = {
	0,                                    /* --- */
	31,                                   /* Jan */
	31+29,                                /* Feb */
	31+29+31,                             /* Mar */
	31+29+31+30,                          /* Apr */
	31+29+31+30+31,                       /* May */
	31+29+31+30+31+30,                    /* Jun */
	31+29+31+30+31+30+31,                 /* Jul */
	31+29+31+30+31+30+31+31,              /* Aug */
	31+29+31+30+31+30+31+31+30,           /* Sep */
	31+29+31+30+31+30+31+31+30+31,        /* Oct */
	31+29+31+30+31+30+31+31+30+31+30,     /* Nov */
	31+29+31+30+31+30+31+31+30+31+30+31   /* Dec */
};

static const uint16 day_offset[] = {
	1,                                    /* Year 0 (leap year) */
	1+2,                                  /* Year 1             */
	1+2+1,                                /* Year 2             */
	1+2+1+1                               /* Year 3             */
};




static uint32 div4x2
(
	uint32 dividend,       /* Dividend, note dword     */
	uint16 divisor,         /* Divisor                  */
	uint16  *rem_ptr    /* Pointer to the remainder */
)
{
	*rem_ptr = (uint16) (dividend % divisor);

	return (dividend / divisor);

} /* END div4x2 */


static void time_jul_from_secs
(
	/* Number of seconds since base date */
	uint32                          secs,

	/* OUT: Pointer to Julian date record */
	time_julian_type                *julian
)
{
	/* Loop index */
	unsigned int /* fast */         i;

	/* Days since beginning of year or quad-year */
	uint16                          days;

	/* Quad years since base date */
	unsigned int /* fast */         quad_years;

	/* Leap-year or non-leap year month table */
	const uint16                    *month_table;

	/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

	/* Add number of seconds from Jan 1 to Jan 6 from input seconds
	   in order to have number of seconds since Jan 1, 1980 for calculation */

	secs += TIME_JUL_OFFSET_S;


	/* Divide elapsed seconds by 60: remainder is seconds of Julian date;
	   quotient is number of elapsed minutes. */

	secs = div4x2 ( secs, 60, &julian->second );


	/* Divide elapsed minutes by 60: remainder is minutes of Julian date;
	   quotient is elapsed hours. */

	secs = div4x2 ( secs, 60, &julian->minute );


	/* Divide elapsed hours by 24: remainder is hours of Julian date;
	   quotient is elapsed days. */

	secs = div4x2 ( secs, 24, &julian->hour );


	/* Now things get messier. We have number of elapsed days. The 1st thing
	   we do is compute how many leap year sets have gone by. We multiply
	   this value by 4 (since there are 4 years in a leap year set) and add
	   in the base year. This gives us a partial year value. */

	quad_years = div4x2( secs, TIME_JUL_QUAD_YEAR, &days );

	julian->year = TIME_JUL_BASE_YEAR + (4 * quad_years);


	/* Now we use the year_tab to figure out which year of the leap year
	   set we are in. */

	for ( i = 0; days >= year_tab[ i + 1 ]; i++ )
	{
		/* No-op. Table seach. */
	}

	/* Subtract out days prior to current year. */
	days -= year_tab[ i ];

	/* Use search index to arrive at current year. */
	julian->year += i;  


	/* Take the day-of-week offset for the number of quad-years, add in
	   the day-of-week offset for the year in a quad-year, add in the number
	   of days into this year. */

	julian->day_of_week =
		(day_offset[3] * quad_years + day_offset[i] + days) % 7;


	/* Now we know year, hour, minute and second. We also know how many days
	   we are into the current year. From this, we can compute day & month. */


	/* Use leap_month_tab in leap years, and norm_month_tab in other years */

	month_table = (i == 0) ? leap_month_tab : norm_month_tab;


	/* Search month-table to compute month */

	for ( i = 0; days >= month_table[ i + 1 ]; i++ )
	{
		/* No-op. Table seach. */
	}


	/* Compute & store day of month. */
	julian->day = days - month_table[ i ] + 1;  

	/* Store month. */
	julian->month = i + 1;


} /* time_jul_from_secs */

time_t get_modem_time_utc_sec()
{
	return 0;
}

int get_modem_time_tm(struct tm* time)
{
	
	return MODEM_TIME_RET_FAIL;
}
