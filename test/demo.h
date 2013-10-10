#ifndef __DEMO_H_H__
#define __DEMO_H_H__

#include <pthread.h>

#include <adapters/libev.h>

struct my_data {
	ev_async *async;
	void *data;
};


class redis_client {
	struct ev_loop *loop_;
	ev_async async_w_;

	void prepare_loop (EV_P);
 public:
	redis_client();

	void attach_io(redisAsyncContext *c);
	void async();
	void redis_command(redisAsyncContext *c, void *pri, const char *key);
	int start();

	void syn_cmd(redisAsyncContext *c0, redisAsyncContext *c1, redisAsyncContext *c2, const char *cmd);

};


#endif
