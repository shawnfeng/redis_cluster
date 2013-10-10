#ifndef __HIREDIS_LIBEV_H__
#define __HIREDIS_LIBEV_H__
#include <stdlib.h>
#include <sys/types.h>

#include <stdint.h>

#include <ev.h>
#include <hiredis.h>
#include <async.h>

typedef struct redisLibevEvents {
	redisAsyncContext *context;
	struct ev_loop *loop;
	int reading, writing;
	ev_io rev, wev;
	uint64_t addr;
	int status; // 0 attach 1 establish
} redisLibevEvents;


int redisLibevAttach(EV_P_ redisAsyncContext *ac, uint64_t addr);

#endif
