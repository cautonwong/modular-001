#include "nidavellir.h"


uint16_t randomize_int(uint16_t interval)
{
	static uint32_t seed = 1;
	uint32_t hi, lo;
	lo = 16807 * (seed & 0xFFFF);
	hi = 16807 * (seed >> 16);
	lo += (hi & 0x7FFF) << 16;
	lo += hi >> 15;
	if (lo > 0x7FFFFFFF) lo -= 0x7FFFFFFF;
	seed = lo;
	return (uint16_t)( ((float)interval * (float)lo) / (float)0x7FFFFFFF );
}

float bound_min_max(float val, float min, float max)
{
	if (val < min)
		return min;
	if (val > max)
		return max;
	return val;
}

/* Test for leap year. Nonzero if YEAR is a leap year (every 4 years,
 except every 100th isn't, and every 400th is). */
# define __isleap(year) \
 ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

/* How many days come before each month (0-12).  */
static const uint16_t __mon_yday[2][13] = {
	/* Normal years.  */
	{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
	/* Leap years.  */
	{0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
};

#define	SECS_PER_HOUR	(60 * 60)
#define	SECS_PER_DAY	(SECS_PER_HOUR * 24)


/**
 * Convert UNIX timestamp to date/time
 * Based on code from GNU C Library
 */
void date_from_timestamp(uint32_t timestamp, DateTimeT *date_time)
{
	uint16_t y;
	int16_t days;
	uint32_t rem;

	days = timestamp / SECS_PER_DAY;
	rem = timestamp % SECS_PER_DAY;

	date_time->hour = rem / SECS_PER_HOUR;
	rem %= SECS_PER_HOUR;
	date_time->min = rem / 60;
	date_time->sec = rem % 60;
	/* January 1, 1970 was a Thursday.  */
	date_time->wday = (4 + days) % 7;
	y = 1970;

#define DIV(a, b) ((a) / (b) - ((a) % (b) < 0))
#define LEAPS_THRU_END_OF(y) (DIV (y, 4) - DIV (y, 100) + DIV (y, 400))

	while (days < 0 || days >= (__isleap (y) ? 366 : 365))
	{
		/* Guess a corrected year, assuming 365 days per year.  */
		uint16_t yg = y + days / 365 - (days % 365 < 0);

		/* Adjust DAYS and Y to match the guessed year.  */
		days -= ((yg - y) * 365
				 + LEAPS_THRU_END_OF (yg - 1)
				 - LEAPS_THRU_END_OF (y - 1));
		y = yg;
	}
	date_time->year = y - 1900;
	if (date_time->year != y - 1900)
	{
		/* The year cannot be represented due to overflow.  */
		return;
	}
	const uint16_t *ip = __mon_yday[__isleap(y)];
	for (y = 11; days < (uint16_t)ip[y]; --y){
		continue;
	}
	days -= ip[y];
	date_time->mon = y;
	date_time->mday = days + 1;
}

uint16_t buf_to_uint16_msb(uint8_t* buf)
{
	return ((uint16_t)buf[0] << 8) + buf[1];
}


uint32_t buf_to_uint32_msb(uint8_t* buf)
{
	return ((uint32_t)buf[0] << 24) + ((uint32_t)buf[1] << 16) + ((uint32_t)buf[2] << 8) + buf[3];
}

uint8_t uint32_to_buf_msb(uint32_t data, uint32_t* buf)
{
	buf[3] = (uint8_t)data;
	buf[2] = (uint8_t)(data >> 8);
	buf[1] = (uint8_t)(data >> 16);
	buf[0] = (uint8_t)(data >> 24);
	return 4;
}

uint8_t uint32_to_buf_lsb(uint32_t data, uint8_t* buf)
{
	buf[0] = (uint8_t)data;
	buf[1] = (uint8_t)(data >> 8);
	buf[2] = (uint8_t)(data >> 16);
	buf[3] = (uint8_t)(data >> 24);
	return 4;
}

uint32_t buf_to_uint32_lsb(uint8_t* buf)
{
	uint32_t data = 0;
	data = ((uint32_t)buf[3] << 24) +
		((uint32_t)buf[2] << 16) +
		((uint32_t)buf[1] << 8) +
		buf[0];
	return data;
}
