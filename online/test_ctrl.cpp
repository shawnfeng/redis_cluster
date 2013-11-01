#include <stdlib.h>

#include <boost/lexical_cast.hpp>

#include "../src/RedisCtrl.h"
using namespace std;

LogOut g_log(NULL, NULL, LogOut::log_info, LogOut::log_warn, LogOut::log_error);

int main (int argc, char **argv)
{
  RedisCtrl rc(&g_log, "10.4.25.15:4180,10.4.25.15:4181,10.4.25.15:4182", "/tx/online");

  for (;;) {
    rc.check();
    sleep(20);
  }

	return 0;
}

