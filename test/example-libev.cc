#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>

#include <hiredis.h>
#include <async.h>
#include <adapters/libev.h>

void getCallback(redisAsyncContext *c, void *r, void *privdata) {
	redisReply *reply = (redisReply *)r;
    if (reply == NULL) return;
    printf("argv[%s]: %s\n", (char*)privdata, reply->str);

    /* Disconnect after receiving the reply to GET */
    redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

static void heart_check(EV_P_ ev_timer *w, int revents)
{
	printf("heart check\n");
	printf("this causes the innermost ev_run to stop iterating\n");
	//	ev_break (EV_A_ EVBREAK_ONE);
}


int main (int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 10020);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return 1;
    }

    ev_timer timeout_watcher;
    ev_timer_init (&timeout_watcher, heart_check, 5, 0.);
    ev_timer_start (EV_DEFAULT_ &timeout_watcher);


    redisLibevAttach(EV_DEFAULT_ c);
    redisAsyncSetConnectCallback(c,connectCallback);
    redisAsyncSetDisconnectCallback(c,disconnectCallback);
    redisAsyncCommand(c, NULL, NULL, "SET key %b", argv[argc-1], strlen(argv[argc-1]));
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET key");
    ev_loop(EV_DEFAULT_ 0);
    printf("loop over\n");
    sleep(3);

    printf("OVER\n");
    return 0;
}
