#include <stdio.h>
#include <stdlib.h>
#include <wrapper/dtg_log.h>
#include "dtg_type.h"

struct 
{
	char *title;
	char *buf;
	int len;
} dump_arg;

void *packet_dump_thread(void *data)
{
#if (0)
	int i;

	LOGU("%s\n", dump_arg.title);
	for(i = 0; i < dump_arg.len; i++) {
		if( !(i % 40) )
			LOGU("\n");
		LOGU("[%02x]", dump_arg.buf[i]);
	}
	LOGU("\n");
	pthread_exit(NULL);
#endif
}


void net_packet_dump(s8 *title, u8* buf, s32 len)
{
#if (0)
	dump_arg.title = title;
	dump_arg.buf = buf;
	dump_arg.len = len;

	pthread_t tid_dump_packet;
	pthread_attr_t attr;

	if(pthread_attr_init(&attr) != 0) {
		fprintf(stderr, "cannot create packet_dump_thread thread\n");
		return -1;
	}
	 
	if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
		fprintf(stderr, "cannot create packet_dump_thread thread\n");
		return -1;
	}

	if (pthread_create(&tid_dump_packet, &attr, packet_dump_thread, NULL) != 0){
		fprintf(stderr, "cannot create packet_dump_thread thread\n");
		return -1;
	}

	if(pthread_attr_destroy(&attr) != 0) {
		fprintf(stderr, "cannot create packet_dump_thread thread\n");
		return -1;
	}
#endif
}

