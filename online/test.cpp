#include <stdlib.h>

#include <boost/lexical_cast.hpp>

#include "OnlineCtrl.h"
#include "../logic_driver/LogicCore.h"

#include "ZookeeperInit.h"
using namespace std;

const int INIT_SLEEP = 2;
const int THREAD_CB_RUN_TIMES = 2;
const int THREAD_NUMS = 1;

const bool IS_GET_SESSIONS_INFO_TEST = true;
const int GET_SESSIONS_INFO_SLEEP = 2;

const int CALL_TIMEOUT = 100;

LogOut g_log(NULL, LogOut::log_debug, LogOut::log_info, LogOut::log_warn, LogOut::log_error);
ZKinit g_zk(&g_log, "10.4.25.15:4180,10.4.25.15:4181,10.4.25.15:4182");

RedisEvent g_re(&g_log);
RedisHash g_rh(&g_log,
               g_zk.handle(),
              "/tx/online/legal_nodes"
               );

OnlineCtrl g_oc(&g_log,
                &g_re,
                &g_rh,

              "/data/home/guangxiang.feng/redis_cluster/script");

int hook_syn(int timeout, long uid, const proto_syn &proto, proto_idx_pair &idx)
{
  return g_oc.syn(timeout, uid, proto, idx);
}

int hook_upidx(int timeout, long uid, const proto_upidx &proto, proto_idx_pair &idx)
{
  int rv = g_oc.upidx(timeout, uid, proto, idx);
  g_log.info("hook_upidx-->rv:%d, sid:%d rid:%d", rv, idx.send_idx, idx.recv_idx);
  return rv;
}

int hook_fin(int timeout, long uid, const proto_fin &proto, std::string &cli_info)
{
  return g_oc.fin(timeout, uid, proto, cli_info);
}

int hook_fin_delay(int timeout, long uid, const proto_fin_delay &proto)
{
  return g_oc.fin_delay(timeout, uid, proto);
}


int hook_timeout_rm(int timeout, int stamp, int count, std::vector< std::pair<long, std::string> > &rvs)
{
  return g_oc.timeout_rm(timeout, stamp, count, rvs);
}

int hook_offline_notify(long uid, std::string &cli_info)
{
  g_log.info("hook_offline_notify-->uid:%ld cli:%s", uid, cli_info.c_str());
  return 0;
}

int hook_offline_notify_multi(std::vector< std::pair<long, std::string> > &rvs)
{
  const char *fun = "hook_offline_notify_multi";
  for (vector< pair<long, string> >::const_iterator it = rvs.begin();
         it != rvs.end(); ++it) {
         g_log.info("%s-->uid:%ld cli:%s", fun, it->first, it->second.c_str());
       }
         
         return 0;
}



//int hook_upidx_fn(int timeout, long uid, const proto_heart &proto);
  LogicCore g_lc(&g_log, hook_syn, hook_fin, hook_fin_delay, hook_upidx, hook_timeout_rm,
                 hook_offline_notify, hook_offline_notify_multi);

void get_multi_test()
{
  long actor = 10;
  map< long, map< string, map<string, string> > > uids_sessions;
  vector<long> uids;
  uids.push_back(actor);
  g_oc.get_multi(CALL_TIMEOUT, actor, uids, uids_sessions);


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

}

void syn_test()
{
  long uid = 10;
  int cli_tp = 100000;
  long conn = 2342134;
  string sublayer_index = "adfasd/adfw";
  map<string, string> kvs;
  kvs["FUCK"] = "beauty";
  g_lc.from_sublayer_synok(uid, conn, cli_tp, "32.33", sublayer_index, kvs);

  conn = 2342135;
  g_lc.from_sublayer_synok(uid, conn, cli_tp, "32.33", sublayer_index, kvs);

}

void fin_test()
{
  int head_len = LogicCore::PROTO_LEN_FIN;
  long conn_idx = 2342134;
  int pro_tp = LogicCore::PROTO_TYPE_FIN;
  int sendidx = 1;
  int recvidx = 0;
  long uid = 10;

  char buff[LogicCore::PROTO_LEN_FIN];
  char *p = buff;

  p = bit32_ltt_stream(head_len, p, LogicCore::PROTO_LEN_HEAD);
  p = bit64_ltt_stream(conn_idx, p, LogicCore::PROTO_LEN_CONN);
  p = bit32_ltt_stream(pro_tp, p, LogicCore::PROTO_LEN_TYPE);
  p = bit32_ltt_stream(sendidx, p, LogicCore::PROTO_LEN_SENDIDX);
  p = bit32_ltt_stream(recvidx, p, LogicCore::PROTO_LEN_RECVIDX);
  p = bit64_ltt_stream(uid, p, LogicCore::PROTO_LEN_UID);

  string sublayer_index = "adfasd/adfw";
  string pro;
  pro.assign(buff, LogicCore::PROTO_LEN_FIN);
  g_lc.from_sublayer(sublayer_index, pro);

}

void fin_delay_test()
{
  int head_len = LogicCore::PROTO_LEN_FIN_DELAY;
  long conn_idx = 2342134;
  int pro_tp = LogicCore::PROTO_TYPE_FIN_DELAY;
  int sendidx = 1;
  int recvidx = 0;
  long uid = 10;

  int delay = time(NULL) + 10;

  char buff[LogicCore::PROTO_LEN_FIN_DELAY];
  char *p = buff;

  p = bit32_ltt_stream(head_len, p, LogicCore::PROTO_LEN_HEAD);
  p = bit64_ltt_stream(conn_idx, p, LogicCore::PROTO_LEN_CONN);
  p = bit32_ltt_stream(pro_tp, p, LogicCore::PROTO_LEN_TYPE);
  p = bit32_ltt_stream(sendidx, p, LogicCore::PROTO_LEN_SENDIDX);
  p = bit32_ltt_stream(recvidx, p, LogicCore::PROTO_LEN_RECVIDX);
  p = bit64_ltt_stream(uid, p, LogicCore::PROTO_LEN_UID);
  p = bit32_ltt_stream(delay, p, LogicCore::PROTO_LEN_STAMP);

  string sublayer_index = "adfasd/adfw";
  string pro;
  pro.assign(buff, LogicCore::PROTO_LEN_FIN_DELAY);
  g_lc.from_sublayer(sublayer_index, pro);

}

void upidx_test()
{
  long conn_idx = 2342134;
  long uid = 10;
  int cli_tp = 100000;
  string sublayer_index = "adfasd/adfw_up";


  int head_len = LogicCore::PROTO_LEN_UPIDX;
  int pro_tp = LogicCore::PROTO_TYPE_UPIDX;
  int sendidx = 1;
  int recvidx = 0;
  char buff[LogicCore::PROTO_LEN_UPIDX];
  char *p = buff;

  p = bit32_ltt_stream(head_len, p, LogicCore::PROTO_LEN_HEAD);
  p = bit64_ltt_stream(conn_idx, p, LogicCore::PROTO_LEN_CONN);
  p = bit32_ltt_stream(pro_tp, p, LogicCore::PROTO_LEN_TYPE);

  p = bit32_ltt_stream(sendidx, p, LogicCore::PROTO_LEN_SENDIDX);
  p = bit32_ltt_stream(recvidx, p, LogicCore::PROTO_LEN_RECVIDX);
  p = bit64_ltt_stream(uid, p, LogicCore::PROTO_LEN_UID);
  p = bit32_ltt_stream(cli_tp, p, LogicCore::PROTO_LEN_CLI_TYPE);
  /*
  for (int i = 0; i < LogicCore::PROTO_LEN_UPIDX; ++i) {
    
    g_log.info("%d %X", i, (unsigned char)buff[i]);
  }
  */


  string pro;
  pro.assign(buff, LogicCore::PROTO_LEN_UPIDX);
  g_lc.from_sublayer(sublayer_index, pro);

}


void *logic_driver_thread_cb(void* args)
{
  for (int i = 0; i < THREAD_CB_RUN_TIMES; ++i) {
    //syn_test();
    //upidx_test();
    //fin_test();
    fin_delay_test();
    //get_multi_test();
    sleep(1);
  }

  return NULL;
}


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

    //    oc->online(CALL_TIMEOUT, uid, session, 100000, kvs);

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

	g_log.debug("%u", sizeof(size_t));
	g_log.debug("%u", sizeof(int));
	g_log.debug("%u", sizeof(long));
	g_log.debug("%u", sizeof(long long));
	g_log.debug("%u", sizeof(long long int));



  sleep(INIT_SLEEP);


	pthread_t pids[THREAD_NUMS];
	int pn = (int)(sizeof(pids)/sizeof(pids[0]));
	for (int i = 0; i < pn; ++i) {
		if (pthread_create(&pids[i], NULL, logic_driver_thread_cb, NULL)) {
			printf("create thread error!\n");
			return -1;  
		}

	}

	for (int i = 0; i < pn; ++i) {
		pthread_join(pids[i],NULL);
	}

  //  pause();


	return 0;
}

