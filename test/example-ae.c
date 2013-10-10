#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include <hiredis.h>
#include <async.h>
#include <adapters/ae.h>

/* Put event loop in the global scope, so it can be explicitly stopped */
static aeEventLoop *loop;

static int count = 0;
static int count1 = 0;

void setCallback(redisAsyncContext *c, void *r, void *privdata) {

	printf("set ok\n");
}

void getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = r;
    if (reply == NULL) return;
    printf("argv[%s]: %s\n", (char*)privdata, reply->str);

    printf("count=%d\n", count++);

    sleep(3);

    /* Disconnect after receiving the reply to GET */
    //redisAsyncDisconnect(c);
}

void getCallback1(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = r;
    if (reply == NULL) return;
    printf("argv[%s]: %s\n", (char*)privdata, reply->str);

    printf("count1=%d\n", count1++);

    sleep(3);

    /* Disconnect after receiving the reply to GET */
    //redisAsyncDisconnect(c);
}


void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        aeStop(loop);
        return;
    }

    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        aeStop(loop);
        return;
    }

    printf("Disconnected...\n");
    aeStop(loop);
}

void func_1(void* args)
{  
	//	redisAsyncContext *c = (redisAsyncContext *)args;
	loop = aeCreateEventLoop(64);

	printf("#################\n");
	aeMain(loop);
	printf("$$$$$$$$$$$$$$$$$\n");
}

struct syn_t {
	pthread_mutex_t *lock;
	pthread_cond_t *con;
	char *buff;
	int size;
};

void getCallbackSyn(redisAsyncContext *c, void *r, void *privdata) {

	printf("call getCallbackSyn 0\n");
	struct syn_t *s = (struct syn_t *)privdata;
	redisReply *reply = r;


	if (reply != NULL) {
		snprintf(s->buff, s->size, "getCallbackSyn:%s", reply->str);
	}
    //    printf("argv[%s]: %s\n", (char*)privdata, reply->str);

	printf("call getCallbackSyn 1\n");
	sleep(3);
	printf("call getCallbackSyn 2\n");
	pthread_mutex_lock(s->lock);
	pthread_cond_signal(s->con);
	pthread_mutex_unlock(s->lock);
	printf("call getCallbackSyn 3\n");

}


void syn_m(redisAsyncContext *c, char *out, int size)
{
	pthread_mutex_t count_lock;
	pthread_cond_t count_nonzero;

	pthread_mutex_init(&count_lock, NULL);
	pthread_cond_init(&count_nonzero, NULL);

	pthread_mutex_lock(&count_lock);

	struct syn_t s;

	s.buff = out;
	s.size = size;
	s.lock = &count_lock;
	s.con = &count_nonzero;

	printf("syn_m 0\n");
	redisAsyncCommand(c, getCallbackSyn, (void *)&s, "GET key");
	printf("syn_m 1\n");
	pthread_cond_wait(&count_nonzero, &count_lock);
	printf("syn_m 2\n");


	pthread_mutex_unlock(&count_lock);
	printf("syn_m 3\n");

}

int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 10020);
    redisAsyncContext *c1 = redisAsyncConnect("127.0.0.1", 10010);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return 1;
    }


    if (c1->err) {
        /* Let *c leak for now... */
        printf("Error1: %s\n", c->errstr);
        return 1;
    }


    pthread_t pid1;
  
    if (pthread_create(&pid1, NULL, (void *)func_1, NULL)) {
	    printf("create thread error!\n");
	    return -1;  
    }

    sleep(3);


    redisAeAttach(loop, c);
    redisAeAttach(loop, c);
    printf("===========\n");
    //sleep(5);


    redisAsyncSetConnectCallback(c,connectCallback);
    redisAsyncSetDisconnectCallback(c,disconnectCallback);
    /*
    //    return 1;
    printf("1st\n");
    redisAsyncCommand(c, setCallback, NULL, "SET key %b_c", argv[argc-1], strlen(argv[argc-1]));
    //redisAsyncCommand(c1, setCallback, NULL, "SET key %b_c1", argv[argc-1], strlen(argv[argc-1]));

    printf("2nd\n");
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET key");
    //redisAsyncCommand(c1, getCallback, (char*)"end-1", "GET key");
    printf("3rd\n");
    redisAsyncCommand(c, getCallback1, (char*)"end-2", "GET key");
    //redisAsyncCommand(c1, getCallback1, (char*)"end-2", "GET key");


    redisAeAttach(loop, c1);
    printf("4th\n");
    redisAsyncCommand(c1, setCallback, NULL, "SET key %b_c1", argv[argc-1], strlen(argv[argc-1]));
    redisAsyncCommand(c1, getCallback, (char*)"end-1", "GET key");
    redisAsyncCommand(c1, getCallback1, (char*)"end-2", "GET key");

    printf("5th\n");
    redisAsyncCommand(c, getCallback1, (char*)"end-2", "GET key");

    */
    char buff[512];
    syn_m(c, buff, sizeof(buff));
    printf("!!!!c buff=%s\n", buff);


    //syn_m(c1, buff, sizeof(buff));
    //printf("!!!!c1 buff=%s\n", buff);


    pthread_join(pid1,NULL);

    printf("OVER\n");
    return 0;
}

