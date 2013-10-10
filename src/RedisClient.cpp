#include "RedisClient.h"

using namespace std;

void RedisClient::cmd(RedisRvs &rv, const std::string &log_key, const std::vector<std::string> &hash,
                      int timeout, const std::vector<std::string> &args,
                      const std::string &lua_code
                      )
{
  TimeUse tu;
  const char *fun = "RedisClient::cmd";
	set<uint64_t> addrs;
	rcx_.hash_addr(hash, addrs);

	log_.info("%s-->k:%s hash tm:%ld", fun, log_key.c_str(), tu.intv_reset());
  /*
  string cmd;
  for (size_t i = 0; i < args.size(); ++i) {
    cmd.append(args[i]);
    cmd.append(" ");
  }

  log_.debug("%s-->k:%s cmd:%s", fun, log_key.c_str(), cmd.c_str());
  */
  re_.cmd(rv, log_key, addrs, timeout, args, lua_code, false);
	log_.info("%s-->k:%s event tm:%ld", fun, log_key.c_str(), tu.intv_reset());

}

void RedisClient::cmd(const std::vector<long> &hash, const char *c, int timeout, std::vector<std::string> &rv)
{

}

