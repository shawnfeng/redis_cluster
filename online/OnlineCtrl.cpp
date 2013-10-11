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

                       const char *script_path
                       ) : log_(log_t, log_d, log_i, log_w, log_e),
                           sp_(script_path),
                           rc_(log_t, log_d, log_i, log_w, log_e,
                               "10.2.72.12:4180,10.2.72.12:4181,10.2.72.12:4182",
                               "/tx/online/legal_nodes"
                               )
{
  rc_.start();

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


void OnlineCtrl::offline(long uid, const std::string &session)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::offline";
	vector<string> hash;
	RedisRvs rv;

  string suid = boost::lexical_cast<string>(uid);
  string log_key = suid;
  hash.push_back(suid);

	int timeout = 100;

  vector<string> args;
  args.push_back("EVALSHA");
  args.push_back(s_offline_.sha1);

  args.push_back("2");
  args.push_back(suid);
  args.push_back(session);

  rc_.cmd(rv, log_key, hash, timeout, args, s_offline_.data);

	log_.debug("%s-->uid:%ld session:%s rv.size:%lu", fun, uid, session.c_str(), rv.size());

	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    string addr = fun + boost::lexical_cast<string>(it->first);
    it->second.dump(&log_, addr.c_str(), 0);
	}

	log_.info("%s-->uid:%ld tm:%ld", fun, uid, tu.intv());

}


void OnlineCtrl::online(long uid,
			const string &session,
			const vector<string> &kvs)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::online";

	vector<string> hash;
	RedisRvs rv;

  string suid = boost::lexical_cast<string>(uid);
  string log_key = suid;
  hash.push_back(suid);

	int timeout = 100;
	int stamp = time(NULL);

	//rc_.cmd(rv, hash, 100, "SCRIPT LOAD %s", data.c_str());
	//rc_.cmd(rv, hash, 100, "EVAL %s %d %s %s %d", data.c_str(), 3, "t0", "t1", stamp);
	//rc_.cmd(rv, hash, 100, "EVALSHA %s %d %s %s", sha1.c_str(), 2, "t0", "t1");

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
  args.push_back(boost::lexical_cast<string>(stamp));

  args.insert(args.end(), kvs.begin(), kvs.end());
	log_.info("%s-->arg uid:%ld tm:%ld", fun, uid, tu.intv_reset());

  rc_.cmd(rv, log_key, hash, timeout, args, s_online_.data);

	log_.info("%s-->get uid:%ld tm:%ld", fun, uid, tu.intv_reset());

	log_.debug("%s-->uid:%ld session:%s rv.size:%lu", fun, uid, session.c_str(), rv.size());

	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    string addr = fun + boost::lexical_cast<string>(it->first);
    it->second.dump(&log_, addr.c_str(), 0);
    /*
    // return test
    if (it->second.integer == uid) {
      log_.info("OOOOOOOOOOOOOOOOOOOO%ld OK", uid);
    } else {
      log_.error("EEEEEEEEEEEEEEEEEEEE%ld ERROR", uid);
    }
    */
	}

	log_.info("%s-->over uid:%ld tm:%ld", fun, uid, tu.intv());


}

void OnlineCtrl::get_sessions(long uid, vector<string> &sessions)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::get_sessions";
	vector<string> hash;
	RedisRvs rv;

  string suid = boost::lexical_cast<string>(uid);
  string log_key = suid;
  hash.push_back(suid);



	int timeout = 100;

  vector<string> args;
  args.push_back("EVALSHA");
  args.push_back(s_sessions_.sha1);

  args.push_back("1");
  args.push_back(suid);

  rc_.cmd(rv, log_key, hash, timeout, args, s_sessions_.data);

	log_.debug("%s-->uid:%ld rv.size:%lu", fun, uid, rv.size());

	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    uint64_t addr = it->first;
    const RedisRv &tmp = it->second;
    string saddr = fun + boost::lexical_cast<string>(addr);
    tmp.dump(&log_, saddr.c_str(), 0);

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


void OnlineCtrl::get_session_info(long uid, const string &session, const vector<string> &ks,
                                  map<string, string> &kvs)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::get_session_info";

	vector<string> hash;
	RedisRvs rv;

  string suid = boost::lexical_cast<string>(uid);
  string log_key = suid;
  hash.push_back(suid);


	int timeout = 100;

  vector<string> args;
  args.push_back("EVALSHA");
  args.push_back(s_session_info_.sha1);

  args.push_back("2");
  args.push_back(suid);
  args.push_back(session);

  args.insert(args.end(), ks.begin(), ks.end());

  rc_.cmd(rv, log_key, hash, timeout, args, s_session_info_.data);

	log_.debug("%s-->uid:%ld session:%s rv.size:%lu", fun, uid, session.c_str(), rv.size());

	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    uint64_t addr = it->first;
    const RedisRv &tmp = it->second;
    string saddr = fun + boost::lexical_cast<string>(addr);
    tmp.dump(&log_, saddr.c_str(), 0);

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


