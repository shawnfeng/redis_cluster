#include <arpa/inet.h>
#include "RedisClient.h"

using namespace std;

static LogOut g_log;


void *thread_cb(void* args)
{  
	RedisClient *rc = (RedisClient *)args;
	vector<string> hash;

	for (int i = 0; i < 2; ++i) {
		vector<string> vs;
		rc->cmd(hash, "GET key0", 100, vs);

		for (vector<string>::const_iterator it = vs.begin(); it != vs.end(); ++it) {
			g_log.debug("=== %s ===", it->c_str());
		}


	}

	return NULL;

}


int main (int argc, char **argv)
{

	//long ip = inet_addr("10.3.2.3");
	//g_log.info("ip=%ld", ip);
	g_log.info("ulong=%lu", sizeof(ulong));
	g_log.info("long=%lu", sizeof(long));
	g_log.info("int=%lu", sizeof(int));
	g_log.info("uint64_t=%lu", sizeof(uint64_t));
	g_log.info("uint32_t=%lu", sizeof(uint32_t));
	g_log.info("in_addr_t=%lu", sizeof(in_addr_t));

	uint64_t ipv4 = ipv4_int64("10.3.2.3", 333);
	g_log.info("ip=%ld", ipv4);

	char buff[100];
	int port;

	int64_ipv4(ipv4, buff, 100, port);
	g_log.info("ip=%s,port=%d", buff, port);


	//=================================
	g_log.info("MAIN-->RediClient init");
	RedisClient rc(LogOut::log_trace, LogOut::log_debug, LogOut::log_info, LogOut::log_warn, LogOut::log_error,
		       //		       "127.0.0.1:4180,127.0.0.1:4181,127.0.0.1:4182",
		       "127.0.0.1:4180,127.0.0.1:4181,127.0.0.1:4182",
		       "/tx/online/legal_nodes"
		       );

	rc.start();
	sleep(2);
	//pause();
	/*
	vector< pair<string, int> > ends;
	ends.push_back(pair<string, int>("127.0.0.1", 10010));
	ends.push_back(pair<string, int>("127.0.0.1", 10020));
	ends.push_back(pair<string, int>("10.2.72.23", 10010));

	rc.update_ends(ends);
	*/
	g_log.info("MAIN-->RedisClient start");
	g_log.info("sleep ZZZ");
	// ============================

	pthread_t pids[2];
	int pn = (int)(sizeof(pids)/sizeof(pids[0]));
	for (int i = 0; i < pn; ++i) {
		if (pthread_create(&pids[i], NULL, thread_cb, (void *)&rc)) {
			printf("create thread error!\n");
			return -1;  
		}

	}

	for (int i = 0; i < pn; ++i) {
		pthread_join(pids[i],NULL);
	}


	g_log.info("MAIN-->hold here");
	pause();
	
	return 1;

}
