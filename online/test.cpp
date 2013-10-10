#include <stdlib.h>

#include "OnlineCtrl.h"
using namespace std;

const int INIT_SLEEP = 2;
const int THREAD_CB_RUN_TIMES = 2;
const int THREAD_NUMS = 1;

const bool IS_GET_SESSIONS_INFO_TEST = true;
const int GET_SESSIONS_INFO_SLEEP = 2;

LogOut g_log;

void *thread_cb(void* args)
{  
	OnlineCtrl *oc = (OnlineCtrl *)args;


	string session = "fuck2";
	vector<string> kvs;
	kvs.push_back("k3");
	kvs.push_back("v3");

	kvs.push_back("asdf");
	kvs.push_back("333");

	kvs.push_back("k3dd");
	kvs.push_back("v333");

	kvs.push_back("124");
	kvs.push_back("v3a33");

	kvs.push_back("k3=lad");
	kvs.push_back("v33d");

	kvs.push_back("k333lad");
	kvs.push_back("v33ddd");


  for (int i = 0; i < THREAD_CB_RUN_TIMES; ++i) {
    long uid = rand() % 100000;
    uid = 68154;
    oc->online(uid, session, kvs);

    if (IS_GET_SESSIONS_INFO_TEST) {
      sleep(GET_SESSIONS_INFO_SLEEP);
      map<string, string> rv;
      oc->get_session_info(uid, session, vector<string>(), rv);

      for (map<string, string>::const_iterator it = rv.begin(); it != rv.end(); ++it) {
        g_log.info("%ld session info %s:%s", uid, it->first.c_str(), it->second.c_str());
      }
    }
  }


	return NULL;

}


int main (int argc, char **argv)
{

	g_log.debug("%u", sizeof(int));
	g_log.debug("%u", sizeof(long));
	g_log.debug("%u", sizeof(long long));
	g_log.debug("%u", sizeof(long long int));


	OnlineCtrl oc(LogOut::log_trace, LogOut::log_debug, LogOut::log_info, LogOut::log_warn, LogOut::log_error,
                "/data/home/guangxiang.feng/code_tst/redis_client/script");

  sleep(INIT_SLEEP);

	pthread_t pids[THREAD_NUMS];
	int pn = (int)(sizeof(pids)/sizeof(pids[0]));
	for (int i = 0; i < pn; ++i) {
		if (pthread_create(&pids[i], NULL, thread_cb, (void *)&oc)) {
			printf("create thread error!\n");
			return -1;  
		}

	}

	for (int i = 0; i < pn; ++i) {
		pthread_join(pids[i],NULL);
	}

  /*
	long uid = 1000;
	string session = "fuck2";
	vector<string> kvs;
	kvs.push_back("k3");
	kvs.push_back("v3");


	sleep(1);
	oc.online(uid, session, kvs);
	g_log.debug("=================");
	sleep(20);
	oc.online(uid, session, kvs);
	//oc.offline(uid, session);

	vector<string> ks;
  oc.get_sessions(uid, ks);

  map<string, string> svs;
  oc.get_session_info(uid, session, ks, svs);

	pause();
  */	

	return 0;
}

