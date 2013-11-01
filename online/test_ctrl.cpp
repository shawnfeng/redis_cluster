#include <stdlib.h>

#include <boost/lexical_cast.hpp>

#include "../src/RedisCtrl.h"
#include "ZookeeperInit.h"
using namespace std;


LogOut g_log(NULL, NULL, LogOut::log_info, LogOut::log_warn, LogOut::log_error);
RedisEvent g_re(&g_log);
ZKinit g_zk(&g_log, "10.4.25.15:4180,10.4.25.15:4181,10.4.25.15:4182");
RedisCtrl g_rc(&g_log, &g_re, g_zk.handle(), "/tx/online");


int main (int argc, char **argv)
{

  //  const char *fun = "main";

  for (;;) {
    g_rc.check();
    sleep(10);
  }

	return 0;
}

