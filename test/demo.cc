#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <unistd.h>

#include <string>
#include <vector>

using namespace std;

#include "demo.h"

struct cflag_t {
	int f;
	void *d;

	cflag_t() : f(0), d(NULL) {}
	void reset() { f = 0; d =NULL; }
};


struct cmd_arg_t {
	ev_timer t;
	pthread_mutex_t lock;
	pthread_cond_t cond;



	vector<string> vs;

};


static void *call_back_loop(void* args)
{
	redis_client *rc = (redis_client *)args;
	rc->start();
	return NULL;
}

static void heart_check(EV_P_ ev_timer *w, int revents)
{
	printf("heart check\n");
	printf("this causes the innermost ev_run to stop iterating\n");
	//	ev_break (EV_A_ EVBREAK_ONE);
}




static void async_call_timeout(EV_P_ ev_timer *w, int revents)
{
	puts("async_call_timeout----------");

}


static void async_call(EV_P_ ev_async *w, int revents)
{
	puts("ev_async----------");

	//	ev_timer t;
	//ev_timer *tw = &t; // core
	ev_timer *tw = new ev_timer;
	ev_timer_init(tw, async_call_timeout, 3, 0.);
	ev_timer_start (EV_A_ tw);

}




static void getCallback(redisAsyncContext *c, void *r, void *privdata) {
	cflag_t *cf = (cflag_t *)privdata;

	printf("cf=%p cf->f=%d cf->d=%p\n", cf, cf->f, cf->d);
	// 超时后防止非法内存访问
	if (cf->f == 0) {
		printf("cf->f=%d return\n", cf->f);
		return;
	}

	char *ag = (char *)cf->d;

	cf->reset();


	redisReply *reply = (redisReply *)r;
	if (reply == NULL) return;


	printf("argv[%s]: %s\n", ag, reply->str);
	
	
	printf("getCallback sleep begin %s\n", ag);
	sleep(1);
	printf("getCallback sleep over %s\n", ag);

	//fflush(stdout);
	/* Disconnect after receiving the reply to GET */
	//redisAsyncDisconnect(c);
}

static void getTimerPlus(EV_P_ ev_timer *w, int revents)
{
	printf("getTimerPlus %p\n", w);
	printf("getTimerPlus active=%d\n", ev_is_active(w));
	printf("getTimerPlus pend=%d\n", ev_is_pending(w));

	cflag_t *cf = (cflag_t *)w->data;



	printf("getTimerPlus cf=%p cf->f=%d cf->d=%p\n", cf, cf->f, cf->d);
	if (cf->f == 0) {
		printf("cf->f=%d return\n", cf->f);
		return;
	} else {
		cmd_arg_t *carg = (cmd_arg_t *)cf->d;

		cf->reset();
		puts("TIMER OVER OVER");
		//ev_timer_stop();
		pthread_mutex_lock(&carg->lock);
		pthread_cond_signal(&carg->cond);
		pthread_mutex_unlock(&carg->lock);
		printf("getTimerPlus OVER GET\n");
	}

}


static void getCallPlus(redisAsyncContext *c, void *r, void *privdata) {
	cflag_t *cf = (cflag_t *)privdata;

	printf("getCallPlus cf=%p cf->f=%d cf->d=%p\n", cf, cf->f, cf->d);
	if (cf->f == 0) {
		printf("cf->f=%d return\n", cf->f);
		return;
	}
	// =======================
	cmd_arg_t *carg = (cmd_arg_t *)cf->d;
	vector<string> &vs = carg->vs;

	if (--(cf->f) == 0) {
		printf("getCallPlus reset 0 cf=%p cf->f=%d cf->d=%p\n", cf, cf->f, cf->d);
		cf->reset();
		printf("getCallPlus reset 1 cf=%p cf->f=%d cf->d=%p\n", cf, cf->f, cf->d);
	}


	redisReply *reply = (redisReply *)r;
	if (reply == NULL) goto cond;


	printf("argv %s\n", reply->str);
	if (reply->str == NULL) {
		vs.push_back("NULL");
	} else {
		vs.push_back(reply->str);
	}

	sleep(1);
	/*	
	printf("getCallPlus sleep begin\n");
	sleep(1);
	printf("getCallPlus sleep over\n");
	*/

 cond:
	if (cf->f == 0) {
		puts("OVER OVER");
		redisLibevEvents *rev = (redisLibevEvents *)c->data;
		/*
		puts("STOP TIMER<<<<");
		//ev_timer_stop(rev->loop, &carg->t); //crash
		puts("STOP TIMER>>>>");
		puts("clear pending>>>>");
		//ev_clear_pending(rev->loop, &carg->t); //crash
		puts("clear pending>>>>");
		*/
		pthread_mutex_lock(&carg->lock);
		pthread_cond_signal(&carg->cond);
		pthread_mutex_unlock(&carg->lock);
		printf("getCallPlus OVER GET\n");
	}

}




redis_client::redis_client() : loop_(NULL)
{

}



struct userdata {
	enum { CUR_CALL_NUM = 100000, };
	pthread_mutex_t lock; /* global loop lock */
	ev_async async_w;
	pthread_t tid;
	//pthread_cond_t invoke_cv;
	size_t idx;
	cflag_t cfg[CUR_CALL_NUM];

	userdata() : idx(0) {}
	cflag_t *get_cf()
	{ 
		cflag_t *cf = &cfg[idx++ % CUR_CALL_NUM];
		printf("get_cf cf=%p cf->f=%d cf->d=%p\n", cf, cf->f, cf->d);

		if (cf->f || cf->d) {
			printf("WARN!!\n");
		}

		return cf;
	}
	//void reset_cf(cflag_t *cf) { cf->f = 0; cf->d = NULL; }
};

static void async_cb (EV_P_ ev_async *w, int revents)
{
	puts("async_cb just used for the side effects");
}

static void l_release (EV_P)
{
	userdata *u = (userdata *)ev_userdata (EV_A);
	pthread_mutex_unlock (&u->lock);
}

static void l_acquire (EV_P)
{
	userdata *u = (userdata *)ev_userdata (EV_A);
	pthread_mutex_lock (&u->lock);
}

void *l_run(void *thr_arg)
{
	struct ev_loop *loop = (struct ev_loop *)thr_arg;

	l_acquire (EV_A);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	ev_run (EV_A_ 0);
	l_release (EV_A);

	return 0;
}

static userdata su;
void redis_client::prepare_loop (EV_P)
{
	// for simplicity, we use a static userdata struct.

	userdata *u = &su;

	ev_async_init (&u->async_w, async_cb);
	ev_async_start (EV_A_ &u->async_w);

	pthread_mutex_init (&u->lock, 0);
	//pthread_cond_init (&u->invoke_cv, 0);

	puts("now associate this with the loop");
	ev_set_userdata (EV_A_ u);
	//	ev_set_invoke_pending_cb (EV_A_ l_invoke);
	ev_set_loop_release_cb (EV_A_ l_release, l_acquire);

	puts("then create the thread running ev_run");
	pthread_create (&u->tid, 0, l_run, EV_A);

}

int redis_client::start()
{
	loop_ = EV_DEFAULT;
	prepare_loop(loop_);
	/*
	ev_timer timeout_watcher;
	//ev_timer_init (&timeout_watcher, heart_check, 5, 0.);
	//ev_timer_start (loop_, &timeout_watcher);
	ev_init(&timeout_watcher, heart_check);
	timeout_watcher.repeat = 2.;
	ev_timer_again(loop_, &timeout_watcher);


	ev_async_init(&async_w_, async_call);
	ev_async_start(loop_, &async_w_);



	printf("#################\n");
	ev_run (loop_, 0);
	printf("$$$$$$$$$$$$$$$$$\n");
	*/

	return 0;
}

void redis_client::async()
{
	ev_async_send(loop_, &async_w_);
}

void redis_client::attach_io(redisAsyncContext *c)
{
	redisLibevAttach(loop_, c);

}



void redis_client::syn_cmd(redisAsyncContext *c0, redisAsyncContext *c1, redisAsyncContext *c2, const char *cmd)
{
	userdata *u = (userdata *)ev_userdata (loop_);


	cmd_arg_t carg;
	pthread_mutex_init(&carg.lock, NULL);
	pthread_cond_init(&carg.cond, NULL);


	pthread_mutex_lock (&u->lock);

	printf("syn_cmd %s>>>>>>>>>>\n", cmd);

	cflag_t *cf = u->get_cf();
	cf->f = 3;
	cf->d = (void *)&carg;

	redisAsyncCommand(c0, getCallPlus, cf, cmd);
	redisAsyncCommand(c1, getCallPlus, cf, cmd);
	redisAsyncCommand(c2, getCallPlus, cf, cmd);

	// add timeout timer
	carg.t.data = (void *)cf;

	printf("TIMER=%p\n", &carg.t);
	ev_timer_init (&carg.t, getTimerPlus, 1, 0.);
	ev_timer_start (loop_, &carg.t);



	printf("syn_cmd %s<<<<<<<<<<\n", cmd);

	ev_async_send (loop_, &u->async_w);
	pthread_mutex_unlock (&u->lock);


	pthread_mutex_lock(&carg.lock);
	pthread_cond_wait(&carg.cond, &carg.lock);
	pthread_mutex_unlock(&carg.lock);


	printf("active=%d\n", ev_is_active(&carg.t));
	printf("pend=%d\n", ev_is_pending(&carg.t));

	pthread_mutex_lock (&u->lock);

	puts("STOP TIMER<<<<");
	// mutil call ok
	printf("active=%d\n", ev_is_active(&carg.t));
	ev_timer_stop(loop_, &carg.t);
	printf("active=%d\n", ev_is_active(&carg.t));
	ev_timer_stop(loop_, &carg.t);
	printf("active=%d\n", ev_is_active(&carg.t));
	ev_timer_stop(loop_, &carg.t);
	printf("active=%d\n", ev_is_active(&carg.t));

	puts("STOP TIMER>>>>");
	puts("clear pending>>>>");
	printf("pend=%d\n", ev_is_pending(&carg.t));
	ev_clear_pending(loop_, &carg.t);
	printf("pend=%d\n", ev_is_pending(&carg.t));
	ev_clear_pending(loop_, &carg.t);
	printf("pend=%d\n", ev_is_pending(&carg.t));
	ev_clear_pending(loop_, &carg.t);
	printf("pend=%d\n", ev_is_pending(&carg.t));
	ev_clear_pending(loop_, &carg.t);
	printf("pend=%d\n", ev_is_pending(&carg.t));
	puts("clear pending>>>>");
	pthread_mutex_unlock (&u->lock);


	const vector<string> &vs = carg.vs;
	for (vector<string>::const_iterator it = vs.begin(); it != vs.end(); ++it) {
		printf("=== %s ===\n", it->c_str());
	}

}

void redis_client::redis_command(redisAsyncContext *c, void *pri, const char *cmd)
{

	userdata *u = (userdata *)ev_userdata (loop_);

	pthread_mutex_lock (&u->lock);
	/*
	ev_timer *timeout_watcher = new ev_timer;
	ev_timer_init (timeout_watcher, heart_check, 5, 0.);
	ev_timer_start (loop_, timeout_watcher);
	*/

	cflag_t *cf = u->get_cf();
	cf->f++;
	cf->d = pri;

	printf("cmd %s>>>>>>>>>>\n", cmd);
	redisAsyncCommand(c, getCallback, (char*)cf, cmd);
	printf("cmd %s<<<<<<<<<<\n", cmd);

	ev_async_send (loop_, &u->async_w);
	pthread_mutex_unlock (&u->lock);

}


int main (int argc, char **argv)
{
	cflag_t tt[100000];
	printf("%lu\n", sizeof(tt)/1024);
	printf("pthread_mutex_t=%lu, pthread_cond_t=%lu, cflag_t=%lu\n",
	       sizeof(pthread_mutex_t), sizeof(pthread_cond_t),
	       sizeof(cflag_t));


	redis_client rc;

	rc.start();
	redisAsyncContext *c1 = redisAsyncConnect("127.0.0.1", 10010);
	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 10020);
	if (c->err) {
		printf("Error: %s\n", c->errstr);
		return 1;
	}
	rc.attach_io(c);
	rc.attach_io(c1);

	rc.redis_command(c, (void *)"0st", "GET key0");
	/*
	rc.redis_command(c, (void *)"1nd", "GET key1");
	rc.redis_command(c, (void *)"4nd", "GET key4");
	rc.redis_command(c, (void *)"6nd", "GET key6");
	rc.redis_command(c, (void *)"2rd", "GET key2");
	rc.redis_command(c, (void *)"3th", "GET key3");
	*/

	//rc.syn_cmd(c, c1, c, "GET key0");


	pthread_join(su.tid, NULL);

	/*
	pthread_t pid;
	if (pthread_create(&pid, NULL, call_back_loop, &rc)) {
		printf("create thread error!\n");
		return 1;
	}
	sleep(2);


	puts("async<<<<<<<<<<");
	rc.async();
	puts("async>>>>>>>>>>");


	redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 10020);
	if (c->err) {
		printf("Error: %s\n", c->errstr);
		return 1;
	}

	rc.attach_io(c);
	printf("attach io ok\n");

	redisAsyncCommand(c, getCallback, (char*)"end-1", "GET key");

	printf("redisAsyncCommand ok\n");


	sleep(10);
	redisAsyncCommand(c, getCallback, (char*)"end-2", "GET key");
	printf("redisAsyncCommand too\n");


	pthread_join(pid,NULL);
	*/
	return 0;
}



