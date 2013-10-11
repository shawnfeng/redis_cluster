#include <openssl/sha.h>

#include <boost/lexical_cast.hpp>

#include "OnlineCtrl.h"

using namespace std;

static const char *online_ope = "/online.lua";
static const char *offline_ope = "/offline.lua";
static const char *get_session_info_ope = "/get_session_info.lua";
static const char *get_session_ope = "/get_session_ope.lua";


static bool check_sha1(const char *path, string &data, string &sha1)
{
	char buf[512];
	SHA_CTX s;
	unsigned char hash[20];

	FILE *fp = fopen(path,"rb");
	if (!fp) return false;

	SHA1_Init(&s);
	int size;
	while ((size=fread(buf, sizeof(char), sizeof(buf), fp)) > 0) {
		SHA1_Update(&s, buf, size);
		data.append(buf, size);
	} 
	SHA1_Final(hash, &s);
	fclose(fp);

	sha1.clear();
	char tmp[10] = {0};
	for (int i=0; i < 20; i++) {
		sprintf (tmp, "%.2x", (int)hash[i]);
		sha1 += tmp;
	}

	return true;
 
}

OnlineCtrl::OnlineCtrl(void (*log_t)(const char *),
                       void (*log_d)(const char *),
                       void (*log_i)(const char *),
                       void (*log_w)(const char *),
                       void (*log_e)(const char *),

                       const char *zk_addr,
                       const char *zk_path,

                       const char *script_path
                       ) : log_(log_t, log_d, log_i, log_w, log_e),
                           re_(&log_), rh_(&log_, &re_, zk_addr, zk_path),
                           sp_(script_path)
{
  re_.start();
  rh_.start();

  // init script
  string path = sp_ + online_ope;
	if (!check_sha1(path.c_str(), s_online_.data, s_online_.sha1)) {
		log_.error("%s error check sha1", path.c_str());
	}

	log_.info("data:%s sha1:%s", s_online_.data.c_str(), s_online_.sha1.c_str());

  path = sp_ + offline_ope;
	if (!check_sha1(path.c_str(), s_offline_.data, s_offline_.sha1)) {
		log_.error("%s error check sha1", path.c_str());
	}
	log_.info("data:%s sha1:%s", s_offline_.data.c_str(), s_offline_.sha1.c_str());

  path = sp_ + get_session_info_ope;
	if (!check_sha1(path.c_str(), s_session_info_.data, s_session_info_.sha1)) {
		log_.error("%s error check sha1", path.c_str());
	}
	log_.info("data:%s sha1:%s", s_session_info_.data.c_str(), s_session_info_.sha1.c_str());

  path = sp_ + get_session_ope;
	if (!check_sha1(path.c_str(), s_sessions_.data, s_sessions_.sha1)) {
		log_.error("%s error check sha1", path.c_str());
	}
	log_.info("data:%s sha1:%s", s_sessions_.data.c_str(), s_sessions_.sha1.c_str());


}

void OnlineCtrl::single_uid_commend(
                                    const char *fun,
                                    int timeout,
                                    const std::string &suid, std::vector<std::string> &args,
                                    const string &lua_code,
                                    RedisRvs &rv
                                    )
{

  uint64_t rd_addr = rh_.redis_addr(suid);
  if (!rd_addr) {
    log_.error("%s-->error hash uid:%s", fun, suid.c_str());
    return;
  }

  map< uint64_t, vector<string> > addr_cmd;

  addr_cmd.insert(pair< uint64_t, vector<string> >(rd_addr, vector<string>())).first->second.swap(args);

  re_.cmd(rv, suid.c_str(), addr_cmd, timeout, lua_code, false);


	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    string addr = fun + boost::lexical_cast<string>(it->first);
    it->second.dump(&log_, addr.c_str(), 0);
	}


}

void OnlineCtrl::offline(int timeout, long uid, const std::string &session)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::offline";

	RedisRvs rv;
  vector<string> args;

  string suid = boost::lexical_cast<string>(uid);

  args.push_back("EVALSHA");
  args.push_back(s_offline_.sha1);
  args.push_back("2");
  args.push_back(suid);
  args.push_back(session);

  single_uid_commend(fun, timeout, suid, args, s_offline_.data, rv);

	log_.info("%s-->uid:%ld tm:%ld", fun, uid, tu.intv());

}


void OnlineCtrl::online(int timeout, long uid,
			const string &session,
			const vector<string> &kvs)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::online";

	RedisRvs rv;

  string suid = boost::lexical_cast<string>(uid);

	size_t sz = kvs.size();
	if (sz < 2 || sz % 2 != 0) {
		log_.error("kvs % 2 0 size:%lu", sz);
		return;
	}

  
  vector<string> args;
  args.push_back("EVALSHA");
  args.push_back(s_online_.sha1);

  args.push_back("3");
  args.push_back(suid);
  args.push_back(session);
  args.push_back(boost::lexical_cast<string>(time(NULL)));
  args.insert(args.end(), kvs.begin(), kvs.end());

	log_.info("%s-->arg uid:%ld tm:%ld", fun, uid, tu.intv_reset());
	log_.debug("%s-->uid:%ld session:%s rv.size:%lu", fun, uid, session.c_str(), rv.size());
  
  single_uid_commend(fun, timeout, suid, args, s_online_.data, rv);

	log_.info("%s-->over uid:%ld tm:%ld", fun, uid, tu.intv());


}

void OnlineCtrl::get_sessions(int timeout, long uid, vector<string> &sessions)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::get_sessions";

	RedisRvs rv;

  string suid = boost::lexical_cast<string>(uid);

  vector<string> args;
  args.push_back("EVALSHA");
  args.push_back(s_sessions_.sha1);
  args.push_back("1");
  args.push_back(suid);


	log_.debug("%s-->uid:%ld rv.size:%lu", fun, uid, rv.size());

  single_uid_commend(fun, timeout, suid, args, s_sessions_.data, rv);  

	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    const RedisRv &tmp = it->second;

    if (tmp.type == REDIS_REPLY_ARRAY) {
      for (vector<RedisRv>::const_iterator jt = tmp.element.begin();
         jt != tmp.element.end(); ++jt) {

        if (jt->type == REDIS_REPLY_STRING) {
          sessions.push_back(jt->str);
        }
    
      }

    }
  }
	log_.info("%s-->uid:%ld tm:%ld", fun, uid, tu.intv());
}


void OnlineCtrl::get_session_info(int timeout, long uid, const string &session, const vector<string> &ks,
                                  map<string, string> &kvs)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::get_session_info";

	RedisRvs rv;

  string suid = boost::lexical_cast<string>(uid);

  vector<string> args;
  args.push_back("EVALSHA");
  args.push_back(s_session_info_.sha1);
  args.push_back("2");
  args.push_back(suid);
  args.push_back(session);
  args.insert(args.end(), ks.begin(), ks.end());

	log_.debug("%s-->uid:%ld session:%s rv.size:%lu", fun, uid, session.c_str(), rv.size());

  single_uid_commend(fun, timeout, suid, args, s_session_info_.data, rv);

	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    uint64_t addr = it->first;
    const RedisRv &tmp = it->second;

    if (tmp.type == REDIS_REPLY_ARRAY) {
      const vector<RedisRv> &tmp_eles = tmp.element;
      bool ispair = true;
      if (tmp_eles.size() % 2 != 0) {
        ispair = false;
      }

      int kvf = 0;
      const char *key = NULL;
      for (vector<RedisRv>::const_iterator jt = tmp_eles.begin();
         jt != tmp_eles.end(); ++jt) {

        if (ispair) {
          if (kvf++ % 2 == 0) {
            key = jt->str.c_str();
          } else {
            kvs[key] = jt->str;
          }

        } else {
          log_.error("%s-->return value not pair addr:%lu uid:%ld session:%s type:%d,int:%ld,str:%s",
                     fun, addr, uid, session.c_str(), jt->type, jt->integer, jt->str.c_str());

        }
      }


    }

    

	}

	log_.info("%s-->uid:%ld tm:%ld", fun, uid, tu.intv());
}


