#ifndef WINDOWS_SUGAR_H
#define WINDOWS_SUGAR_H

/*
	for define functions depand on windows.
*/

struct timezone
{
	int  tz_minuteswest; /* minutes W of Greenwich */
	int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

long getSystemTime();

#endif