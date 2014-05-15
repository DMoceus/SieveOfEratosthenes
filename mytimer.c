#include <sys/time.h>
#include <stdio.h>

struct timeval tv;
unsigned int begin, end;

unsigned int getTime();
void startTiming();
void stopTiming();
void executionTime();
void secondTime();

unsigned int getTime()
{
	gettimeofday(&tv, NULL);
	return (tv.tv_usec)+(tv.tv_sec*1000000);
}

void startTiming()
{
	gettimeofday(&tv, NULL);
	begin = (tv.tv_usec)+(tv.tv_sec*1000000);
}

void stopTiming()
{
	gettimeofday(&tv, NULL);
	end = (tv.tv_usec)+(tv.tv_sec*1000000);
}

void executionTime()
{
	fprintf(stderr,"Execution Time: %u microseconds\n", end-begin);
}

void secondTime()
{
	fprintf(stderr,"Time: %f seconds\n",(float)(end-begin)/1000000);
}
