#include "libev.h"
#include "RedisEvent.h"

static void redisLibevReadEvent(EV_P_ ev_io *watcher, int revents) {
	//printf("$$$$call redisLibevReadEvent\n");
#if EV_MULTIPLICITY
	//printf("$$$$redisLibevReadEvent define EV_MULTIPLICITY\n");
	((void)loop);
#endif
	((void)revents);

	redisLibevEvents *e = (redisLibevEvents*)watcher->data;
	redisAsyncHandleRead(e->context);
}

static void redisLibevWriteEvent(EV_P_ ev_io *watcher, int revents) {
	//printf("$$$$call redisLibevWriteEvent\n");
#if EV_MULTIPLICITY
	((void)loop);
#endif
	((void)revents);

	redisLibevEvents *e = (redisLibevEvents*)watcher->data;
	redisAsyncHandleWrite(e->context);
}

static void redisLibevAddRead(void *privdata) {
	//printf("$$$$call redisLibevAddRead\n");
	redisLibevEvents *e = (redisLibevEvents*)privdata;
	struct ev_loop *loop = e->loop;
	((void)loop);
	if (!e->reading) {
		e->reading = 1;
		ev_io_start(EV_A_ &e->rev);
	}
}

static void redisLibevDelRead(void *privdata) {
	//printf("$$$$call redisLibevDelRead\n");
	redisLibevEvents *e = (redisLibevEvents*)privdata;
	struct ev_loop *loop = e->loop;
	((void)loop);
	if (e->reading) {
		e->reading = 0;
		ev_io_stop(EV_A_ &e->rev);
	}
}

static void redisLibevAddWrite(void *privdata) {
	//printf("$$$$call redisLibevAddWrite\n");
	redisLibevEvents *e = (redisLibevEvents*)privdata;
	struct ev_loop *loop = e->loop;
	((void)loop);
	if (!e->writing) {
		e->writing = 1;
		ev_io_start(EV_A_ &e->wev);
	}
}

static void redisLibevDelWrite(void *privdata) {
	//printf("$$$$call redisLibevDelWrite\n");
	redisLibevEvents *e = (redisLibevEvents*)privdata;
	struct ev_loop *loop = e->loop;
	((void)loop);
	if (e->writing) {
		e->writing = 0;
		ev_io_stop(EV_A_ &e->wev);
	}
}

static void redisLibevCleanup(void *privdata) {
	//	printf("$$$$call redisLibevCleanup\n");
	redisLibevEvents *e = (redisLibevEvents*)privdata;
	redisAsyncContext *context = e->context;

	userdata_t *u = (userdata_t *)ev_userdata(EV_DEFAULT);
	RedisEvent *re = u->re();
	assert(re);

	re->log()->trace("libev::redisLibevCleanup-->c:%p e:%p e2:%p addr:%ld\n", context, e, context->ev.data, e->addr);
	redisLibevDelRead(privdata);
	redisLibevDelWrite(privdata);

	u->clear(e->addr);	

  free(e);
	context->ev.data = NULL;
}

int redisLibevAttach(EV_P_ redisAsyncContext *ac, uint64_t addr) {
	//printf("$$$$call redisLibevAttach\n");
	redisContext *c = &(ac->c);
	redisLibevEvents *e;

	/* Nothing should be attached when something is already attached */
	if (ac->ev.data != NULL)
		return REDIS_ERR;

	/* Create container for context and r/w events */
	e = (redisLibevEvents*)malloc(sizeof(*e));
	e->context = ac;
#if EV_MULTIPLICITY
	e->loop = loop;
#else
	e->loop = NULL;
#endif
	e->reading = e->writing = 0;
	e->rev.data = e;
	e->wev.data = e;
	e->addr = addr;
	e->status = 0;
	//    e->data = data;

	userdata_t *u = (userdata_t *)ev_userdata(EV_DEFAULT);
  u->insert(addr, ac);

	/* Register functions to start/stop listening for events */
	ac->ev.addRead = redisLibevAddRead;
	ac->ev.delRead = redisLibevDelRead;
	ac->ev.addWrite = redisLibevAddWrite;
	ac->ev.delWrite = redisLibevDelWrite;
	ac->ev.cleanup = redisLibevCleanup;
	ac->ev.data = e;

	/* Initialize read/write events */
	ev_io_init(&e->rev,redisLibevReadEvent,c->fd,EV_READ);
	ev_io_init(&e->wev,redisLibevWriteEvent,c->fd,EV_WRITE);
	return REDIS_OK;
}


