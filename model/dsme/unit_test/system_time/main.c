#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

int main()
{
	struct tm *local;
	time_t t;
	struct timespec ts;
	int ret;

	//t = time(NULL);
	//local = localtime(&t);

	while(1) {
		ret = clock_gettime(CLOCK_MONOTONIC, &ts); //this function need to add -lrt (gcc main.c -lrt)
		printf("ts.tv_sec = [%ld]\n", ts.tv_usec);
		local = localtime(&ts.tv_usec);

		/*
		t = time(NULL);
		printf("ts.tv_sec = [%ld]\n", t);
		local = localtime(&t);
		*/

		printf("local->tm_year = [%d]\n", local->tm_year);
		printf("local->tm_year = [%d]\n", local->tm_year+1900);
		sleep(1);
	}
	return 0;

}