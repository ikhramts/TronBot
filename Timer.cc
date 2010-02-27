#include "Timer.h"

/*
 * Two separate implementations.  A Linux implementation uses gettimeofday(),
 * which is the actual method of timing the contest entry.  Unfortunately, 
 * that is not available on Windows, so for testing on my computer I use clock().
 */

#ifdef TEST_ENVIRONMENT
	#include <iostream>

	#include <time.h>
	clock_t gTimeOut = 0;
	clock_t gStartTime = 0;

	void SetTimeOut(double seconds) {
		gTimeOut = static_cast<clock_t>(static_cast<double>(CLOCKS_PER_SEC) * seconds - 0.4);
		gStartTime = clock();
	}

	bool HasTimedOut() {
		return ((clock() - gStartTime) >= gTimeOut);
	}

#else /* #ifdef  TEST_ENVIRONMENT */
	#include <sys/time.h>

	//In microseconds
	long int gTimeOut = 0;
	long int gStartTime = 0;
	const long int SECONDS_PER_DAY = 86400;

	void SetTimeOut(double seconds) {
		timeval startTime;
		gettimeofday(&startTime, NULL);

		gStartTime = (startTime.tv_sec % SECONDS_PER_DAY)* 1000 + startTime.tv_usec / 1000;
		gTimeOut = static_cast<long int>(seconds * 1000.0);
	}
	
	bool HasTimedOut() {
		timeval currentTime;
		gettimeofday(&currentTime, NULL);
		
		long int currentMilliseconds = (currentTime.tv_sec % SECONDS_PER_DAY) * 1000 + currentTime.tv_usec / 1000;

		return ((currentMilliseconds - gStartTime) > gTimeOut);
	}
#endif /* #ifdef TEST_ENVIRONMENT */
