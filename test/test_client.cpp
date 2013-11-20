#include <arpa/inet.h>
#include "../src/RedisEvent.h"

using namespace std;


static LogOut g_log(LogOut::log_trace, LogOut::log_debug, LogOut::log_info, LogOut::log_warn, LogOut::log_error);
static RedisEvent g_re(&g_log);

void callback(const RedisRvs &rv, double tm, bool istmout, long key, void *data)
{
  g_log.info("############### tm:%lf istmout:%d key:%ld data:%s rvsize:%lu",
             tm, istmout, key, (char *)data, rv.size());
	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    const RedisRv &tmp = it->second; 
    tmp.dump(&g_log, "out", 0);
  }

}

void test_async()
{
  TimeUse tu;
  RedisRvs rv;
  map< uint64_t, vector<string> > addr_cmd;
  uint64_t a = 2240545255613015845;
  vector<string> cs;
  cs.push_back("KEYS");
  cs.push_back("*");
  addr_cmd[a] = cs;
  g_re.cmd_async(1222, (void *)"FUCK", callback, "test_async", addr_cmd, 1);

  g_log.info("##########tm:%lu", tu.intv());

}

void test_sync()
{
  RedisRvs rv;
  map< uint64_t, vector<string> > addr_cmd;
  uint64_t a = 2240545255613015845;
  vector<string> cs;
  cs.push_back("KEYS");
  cs.push_back("U.*");
  addr_cmd[a] = cs;
  g_re.cmd(rv, "test", addr_cmd, 20, "", false);

	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    const RedisRv &tmp = it->second; 
    tmp.dump(&g_log, "out", 0);
  }



}


void *thread_cb(void* args)
{
  for (int i = 0; i < 2; ++i) {
    test_async();

    sleep(1);
  }

	return NULL;
}


int main (int argc, char **argv)
{
  /*
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
  */

	//=================================


	pthread_t pids[1];
	int pn = (int)(sizeof(pids)/sizeof(pids[0]));
	for (int i = 0; i < pn; ++i) {
		if (pthread_create(&pids[i], NULL, thread_cb, NULL)) {
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
