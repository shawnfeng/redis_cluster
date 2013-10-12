#include <stdlib.h>

#include <boost/lexical_cast.hpp>

#include "OnlineCtrl.h"
using namespace std;

const int INIT_SLEEP = 2;
const int THREAD_CB_RUN_TIMES = 2;
const int THREAD_NUMS = 1;

const bool IS_GET_SESSIONS_INFO_TEST = true;
const int GET_SESSIONS_INFO_SLEEP = 2;

const int CALL_TIMEOUT = 100;

LogOut g_log;

static void test_get_multi(OnlineCtrl *oc)
{
  vector<long> uids;
  for (int i = 0; i < 1; ++i) {
    uids.push_back(i);
  }

  for (int i = 0; i < THREAD_CB_RUN_TIMES; ++i) {
    long actor = 1;
    map< long, map< string, map<string, string> > > uids_sessions;
    oc->get_multi(CALL_TIMEOUT, actor, uids, uids_sessions);
    g_log.info("----------test_get_multi 1 size %lu", uids_sessions.size());
    for (map< long, map< string, map<string, string> > >::const_iterator it = uids_sessions.begin();
         it != uids_sessions.end(); ++it) {
      g_log.info("----------test_get_multi 2 size %lu %ld", it->second.size(), it->first);
      for (map< string, map<string, string> >::const_iterator jt = it->second.begin();
           jt != it->second.end(); ++jt) {

        g_log.info("----------test_get_multi 3 size %lu %s", jt->second.size(), jt->first.c_str());
        for (map<string, string>::const_iterator kt = jt->second.begin();
             kt != jt->second.end(); ++kt) {
          g_log.info("test_get_multi %ld %s %s:%s", it->first, jt->first.c_str(), kt->first.c_str(), kt->second.c_str());
        }

      }

    }
    sleep(1);
  }
}

void *thread_cb(void* args)
{  
	OnlineCtrl *oc = (OnlineCtrl *)args;

  test_get_multi(oc);
  return NULL;

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
    kvs.push_back(boost::lexical_cast<string>(uid));
    kvs.push_back(boost::lexical_cast<string>(uid));
    g_log.info("******%ld==============================", uid);

    oc->online(CALL_TIMEOUT, uid, session, kvs);

    if (IS_GET_SESSIONS_INFO_TEST) {
    g_log.info("******%ld--------------------", uid);
      sleep(GET_SESSIONS_INFO_SLEEP);
      map<string, string> rv;
      oc->get_session_info(CALL_TIMEOUT, uid, session, vector<string>(), rv);

      for (map<string, string>::const_iterator it = rv.begin(); it != rv.end(); ++it) {
        g_log.info("%ld session info %s:%s", uid, it->first.c_str(), it->second.c_str());
      }
    }

    kvs.pop_back();
    kvs.pop_back();
  }


	return NULL;

}


int main (int argc, char **argv)
{

	g_log.debug("%u", sizeof(int));
	g_log.debug("%u", sizeof(long));
	g_log.debug("%u", sizeof(long long));
	g_log.debug("%u", sizeof(long long int));


	OnlineCtrl oc(NULL, LogOut::log_debug, LogOut::log_info, LogOut::log_warn, LogOut::log_error,

                "10.2.72.12:4180,10.2.72.12:4181,10.2.72.12:4182",
                "/tx/online/legal_nodes",

                "/data/home/guangxiang.feng/redis_cluster/script");

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

