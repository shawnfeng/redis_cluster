#include <openssl/sha.h>

#include <boost/lexical_cast.hpp>

#include "OnlineCtrl.h"

using namespace std;
/*
static const char *online_ope = "/online.lua";
static const char *offline_ope = "/offline.lua";
*/
static const char *get_session_info_ope = "/get_session_info.lua";
static const char *get_session_ope = "/get_session_ope.lua";
static const char *get_multi_ope = "/get_multi.lua";
static const char *get_timeout_ope = "/timeout_rm.lua";

static const char *syn_ope = "/syn.lua";
static const char *fin_ope = "/fin.lua";
static const char *fin_delay_ope = "/fin_delay.lua";
static const char *upidx_ope = "/upidx.lua";

static const int REQ_SYN = 1;
static const int REQ_FIN = 2;
static const int REQ_FIN_DELAY = 3;
static const int REQ_UPIDX = 4;


typedef void (*cmd_cb_t)(const RedisRvs &, double, bool, long, void *);

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


static void cmd_cb_syn(const RedisRvs &rv, double tm, bool istmout, long uid, void *oc)
{
  OnlineCtrl *me = (OnlineCtrl *)oc;
  if (istmout) {
    me->log()->warn("cmd_cb_syn tm:%lf istmout:%d uid:%ld data:%p rvsize:%lu",
                    tm, istmout, uid, oc, rv.size());
  }

  me->check_scritp_load(REQ_SYN, rv);
  proto_idx_pair idx;
  me->rv_check_syn(uid, rv, idx);

}

static void cmd_cb_fin(const RedisRvs &rv, double tm, bool istmout, long uid, void *oc)
{
  OnlineCtrl *me = (OnlineCtrl *)oc;
  if (istmout) {
    me->log()->warn("cmd_cb_fin tm:%lf istmout:%d uid:%ld data:%p rvsize:%lu",
                    tm, istmout, uid, oc, rv.size());
  }

  me->check_scritp_load(REQ_FIN, rv);
  string cli_info;
  me->rv_check_fin(uid, rv, cli_info);

}

static void cmd_cb_fin_delay(const RedisRvs &rv, double tm, bool istmout, long uid, void *oc)
{
  OnlineCtrl *me = (OnlineCtrl *)oc;
  if (istmout) {
    me->log()->warn("cmd_cb_fin_delay tm:%lf istmout:%d uid:%ld data:%p rvsize:%lu",
                    tm, istmout, uid, oc, rv.size());
  }


  me->check_scritp_load(REQ_FIN_DELAY, rv);
  me->rv_check_fin_delay(uid, rv);
}

static void cmd_cb_upidx(const RedisRvs &rv, double tm, bool istmout, long uid, void *oc)
{
  OnlineCtrl *me = (OnlineCtrl *)oc;
  if (istmout) {
    me->log()->warn("cmd_cb_upidx tm:%lf istmout:%d uid:%ld data:%p rvsize:%lu",
                    tm, istmout, uid, oc, rv.size());
  }
  me->check_scritp_load(REQ_UPIDX, rv);

  proto_idx_pair idx;
  me->rv_check_upidx(uid, rv, idx);

}


const std::string &script_t::lookup_sha1(RedisEvent *re)
{
  const char *fun = "script_t::lookup_sha1";
  if (need_load_) {
    set<uint64_t> addrs;
    {
      boost::unique_lock< boost::shared_mutex > lock(mux_);
      addrs.swap(addrs_);
    }

    vector<string> cmds;
    cmds.push_back("SCRIPT");
    cmds.push_back("LOAD");
    cmds.push_back(data_);

    map< uint64_t, vector<string> > addr_cmd;
    for (set<uint64_t>::const_iterator it = addrs.begin(); it != addrs.end(); ++it) {
      addr_cmd[*it] = cmds;
    }

    if (!addr_cmd.empty()) {
      RedisRvs rv;
      re->cmd(rv, "loadscript", addr_cmd, 100, "", false);

      for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
        const RedisRv &tmp = it->second; 
        tmp.dump(re->log(), fun, 2);
      }


      re->log()->warn("%s-->sha1:%s data:%s", fun, sha1_.c_str(), data_.c_str());
    }


    need_load_ = false;

  }


  return sha1_;
}


void OnlineCtrl::load_script(const std::string &path, script_t &scp)
{
  string data, sha1;
	if (!check_sha1(path.c_str(), data, sha1)) {
		log_->error("%s error check sha1", path.c_str());
	}

  scp.set_scp(sha1.c_str(), data.c_str());

	log_->info("data:%s sha1:%s", scp.data().c_str(), scp.sha1().c_str());

}

OnlineCtrl::OnlineCtrl(
                       LogOut *log,

                       RedisEvent *re,
                       RedisHash *rh,

                       const char *script_path
                       ) : log_(log),
                           re_(re), rh_(rh),
                           sp_(script_path)
{
  //  re_->start();
  //  rh_->start();

  // init script
  load_script(sp_ + get_session_info_ope, s_session_info_);
  load_script(sp_ + get_session_ope, s_sessions_);
  load_script(sp_ + get_multi_ope, s_multi_);
  load_script(sp_ + get_timeout_ope, s_timeout_rm_);

  load_script(sp_ + syn_ope, s_syn_);
  load_script(sp_ + fin_ope, s_fin_);
  load_script(sp_ + fin_delay_ope, s_fin_delay_);
  load_script(sp_ + upidx_ope, s_upidx_);





}

int OnlineCtrl::syn_async(double timeout, long uid, const proto_syn &proto)
{
  const char *fun = "OnlineCtrl::syn_async";
  const vector<string> &kvs = proto.data;
	size_t sz = kvs.size();
	if (sz % 2 != 0) {
		log_->error("kvs % 2 0 size:%lu", sz);
		return 1;
	}

  vector<string> args;
  gen_redis_args(REQ_SYN, uid, &proto, args);

  args.insert(args.end(), kvs.begin(), kvs.end());

  single_uid_commend_async(fun, uid, REQ_SYN, timeout, args);

  return 0;
}

int OnlineCtrl::fin_async(double timeout, long uid, const proto_fin &proto)
{
  const char *fun = "OnlineCtrl::fin_async";
  vector<string> args;
  gen_redis_args(REQ_FIN, uid, &proto, args);

  single_uid_commend_async(fun, uid, REQ_FIN, timeout, args);

  return 0;

}

int OnlineCtrl::fin_delay_async(double timeout, long uid, const proto_fin_delay &proto)
{
  const char *fun = "OnlineCtrl::fin_delay_async";
  vector<string> args;
  gen_redis_args(REQ_FIN_DELAY, uid, &proto, args);

  single_uid_commend_async(fun, uid, REQ_FIN_DELAY, timeout, args);

  return 0;

}

int OnlineCtrl::upidx_async(double timeout, long uid, const proto_upidx &proto)
{
  const char *fun = "OnlineCtrl::upidx_async";
  vector<string> args;
  gen_redis_args(REQ_UPIDX, uid, &proto, args);

  single_uid_commend_async(fun, uid, REQ_UPIDX, timeout, args);

  return 0;
}

void OnlineCtrl::check_scritp_load(int type, const RedisRvs &rv)
{
  if (rv.size() == 1) {
    const RedisRv &tmp = rv.begin()->second;
    if (tmp.type == REDIS_REPLY_ERROR
        && !strncmp(tmp.str.c_str(), "NOSCRIPT", 8)) {

      need_load_script(type, rv.begin()->first);
    }
  }
}



void OnlineCtrl::need_load_script(int type, uint64_t addr)
{
  const char *fun = "OnlineCtrl::need_load_script";
  if (REQ_SYN == type) {
    s_syn_.add(addr);

  } else if (REQ_FIN == type) {
    s_fin_.add(addr);

  } else if (REQ_FIN_DELAY == type) {
    s_fin_delay_.add(addr);


  } else if (REQ_UPIDX == type) {
    s_upidx_.add(addr);

  } else {
    log_->error("%s-->fuck what do you want to load %d!", fun, type);
  }

}

void OnlineCtrl::single_uid_commend_async(const char *fun,
                                          long uid,
                                          int type,
                                          double timeout,
                                          std::vector<std::string> &args
                                          )
{
  uint64_t rd_addr = rh_->redis_addr_master(boost::lexical_cast<string>(uid));
  if (!rd_addr) {
    log_->error("%s-->error hash uid:%ld", fun, uid);
    return;
  }

  map< uint64_t, vector<string> > addr_cmd;

  addr_cmd.insert(pair< uint64_t, vector<string> >(rd_addr, vector<string>())).first->second.swap(args);

  cmd_cb_t cb = NULL;
  if (REQ_SYN == type) {
    cb = cmd_cb_syn;

  } else if (REQ_FIN == type) {
    cb = cmd_cb_fin;

  } else if (REQ_FIN_DELAY == type) {
    cb = cmd_cb_fin_delay;

  } else if (REQ_UPIDX == type) {
    cb = cmd_cb_upidx;

  } else {
    log_->error("%s-->fuck what do you want %ld!", fun, uid);
    return;
  }


  re_->cmd_async(
                 uid, this, cb,
                 "sync",
                 addr_cmd,
                 timeout
                 );

}


void OnlineCtrl::single_uid_commend(
                                    const char *fun,
                                    int timeout,
                                    const std::string &suid, std::vector<std::string> &args,
                                    const string &lua_code,
                                    RedisRvs &rv
                                    )
{

  uint64_t rd_addr = rh_->redis_addr_master(suid);
  if (!rd_addr) {
    log_->error("%s-->error hash uid:%s", fun, suid.c_str());
    return;
  }

  map< uint64_t, vector<string> > addr_cmd;

  addr_cmd.insert(pair< uint64_t, vector<string> >(rd_addr, vector<string>())).first->second.swap(args);

  re_->cmd(rv, suid.c_str(), addr_cmd, timeout, lua_code, false);

  /*
	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    string addr = fun + boost::lexical_cast<string>(it->first);
    it->second.dump(log_, addr.c_str(), 0);
	}
  */

}
void OnlineCtrl::get_sessions(int timeout, long uid, vector<string> &sessions)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::get_sessions";

	RedisRvs rv;

  string suid = boost::lexical_cast<string>(uid);

  vector<string> args;
  args.push_back("EVALSHA");
  args.push_back(s_sessions_.sha1());
  args.push_back("1");
  args.push_back(suid);


	log_->debug("%s-->uid:%ld rv.size:%lu", fun, uid, rv.size());

  single_uid_commend(fun, timeout, suid, args, s_sessions_.data(), rv);  

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
	log_->info("%s-->uid:%ld tm:%ld", fun, uid, tu.intv());
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
  args.push_back(s_session_info_.sha1());
  args.push_back("2");
  args.push_back(suid);
  args.push_back(session);
  args.insert(args.end(), ks.begin(), ks.end());

	log_->debug("%s-->uid:%ld session:%s rv.size:%lu", fun, uid, session.c_str(), rv.size());

  single_uid_commend(fun, timeout, suid, args, s_session_info_.data(), rv);

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
          log_->error("%s-->return value not pair addr:%lu uid:%ld session:%s type:%d,int:%ld,str:%s",
                     fun, addr, uid, session.c_str(), jt->type, jt->integer, jt->str.c_str());

        }
      }


    }

    

	}

	log_->info("%s-->uid:%ld tm:%ld", fun, uid, tu.intv());
}

void OnlineCtrl::one_uid_session(long actor, const RedisRv &tmp, std::map< long, std::map< std::string, std::map<std::string, std::string> > > &uids_sessions)
{
    const char *fun = "OnlineCtrl::one_uid_session";

      if (tmp.type != REDIS_REPLY_ARRAY
          || tmp.element.size() != 3
          || tmp.element.at(0).type != REDIS_REPLY_INTEGER
          || tmp.element.at(1).type != REDIS_REPLY_STRING
          || tmp.element.at(2).type != REDIS_REPLY_ARRAY
          ) {
        log_->error("%s-->retrun sessions invalid format actor:%ld", fun, actor);
        return;
      }

      map< string, map<string, string> > &msessions =
        uids_sessions.insert(
                             pair< long, map< string, map<string, string> > >
                             (tmp.element.at(0).integer, map< string, map<string, string> >() )
                               ).first->second;
      map<string, string> &sessions = msessions.insert(
                                                       pair< string, map<string, string> >
                                                         (tmp.element.at(1).str, map<string, string>() )
                                                          ).first->second;

      const vector<RedisRv> &tmp_eles = tmp.element.at(2).element;

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
            sessions[key] = jt->str;
          }
        } else {
          log_->error("%s-->retrun sessions not pair actor:%ld", fun, actor);

        }

      }


}

void OnlineCtrl::get_multi(int timeout, long actor, const vector<long> &uids,
                           std::map< long, map< std::string, std::map<std::string, std::string> > > &uids_sessions
                           )
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::get_multi";
  string sactor = boost::lexical_cast<string>(actor);
  map< uint64_t, set<string> > disp_uids;

  for (vector<long>::const_iterator it = uids.begin();
       it != uids.end(); ++it) {
    long uid = *it;
    string suid = boost::lexical_cast<string>(uid);
    uint64_t rd_addr = rh_->redis_addr_slave_first(suid);
    if (!rd_addr) {
      log_->error("%s-->acotor:%s error hash uid:%s", fun, sactor.c_str(), suid.c_str());
      continue;
    }

    disp_uids.insert(
                     pair< uint64_t, set<string> >(rd_addr, set<string>())
                     ).first->second.insert(suid);

  }

  log_->info("%s-->addr.size:%lu uids.size:%lu", fun, disp_uids.size(), uids.size());
  map< uint64_t, vector<string> > addr_cmd;

  for (map< uint64_t, set<string> >::const_iterator it = disp_uids.begin();
       it != disp_uids.end(); ++it) {
    vector<string> &args = addr_cmd.insert(pair< uint64_t, vector<string> >(it->first, vector<string>())).first->second;
    args.push_back("EVALSHA");
    args.push_back(s_multi_.sha1());
    args.push_back(boost::lexical_cast<string>(it->second.size()));

    for (set<string>::const_iterator jt = it->second.begin();
         jt != it->second.end(); jt++) {
      args.push_back(*jt);
    }

    log_->info("%s-->addr:%lu uids.size:%lu", fun, it->first, args.size()-3);
  }

  RedisRvs rv;
  re_->cmd(rv, sactor.c_str(), addr_cmd, timeout, s_multi_.data(), false);
  // ================================
  for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    uint64_t addr = it->first;
    const RedisRv &tmp = it->second;
    string saddr = fun + boost::lexical_cast<string>(addr);
    tmp.dump(log_, saddr.c_str(), 0);

    if (tmp.type != REDIS_REPLY_ARRAY) {
      log_->error("%s-->retrun sessions invalid format actor:%ld addr:%lu", fun, actor, addr);
      continue;
    }



    const vector<RedisRv> &tmp_eles = tmp.element;
    for (vector<RedisRv>::const_iterator jt = tmp_eles.begin();
         jt != tmp_eles.end(); ++jt) {
      one_uid_session(actor, *jt, uids_sessions);      
    }


  }
  log_->info("%s-->actor:%ld tm:%ld", fun, actor, tu.intv());
}


int OnlineCtrl::timeout_rm(int timeout, int stamp, int count, std::vector< std::pair<long, std::string> > &rvs)
{
  TimeUse tu;
  const char *fun = "OnlineCtrl::timeout_rm";

  map<int, uint64_t> all_redis;
  rh_->redis_all(all_redis);
  const script_t &script = s_timeout_rm_;
  map< uint64_t, vector<string> > addr_cmd;

  for (map<int, uint64_t>::const_iterator it = all_redis.begin(); it != all_redis.end(); ++it) {
    vector<string> &args = addr_cmd.insert(pair< uint64_t, vector<string> >(it->second, vector<string>())).first->second;    

    args.push_back("EVALSHA");
    args.push_back(script.sha1());
    args.push_back("2");
    args.push_back(boost::lexical_cast<string>(stamp));
    args.push_back(boost::lexical_cast<string>(count));

  }

  RedisRvs rv;
  re_->cmd(rv, "timeout_rm", addr_cmd, timeout, script.data(), false);


  for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    uint64_t addr = it->first;
    const RedisRv &tmp = it->second;
    string saddr = fun + boost::lexical_cast<string>(addr);

    if (tmp.type != REDIS_REPLY_ARRAY
        || tmp.element.size() % 2 != 0
          ) {
      tmp.dump(log_, saddr.c_str(), 3);
      log_->error("%s-->retrun timeoutrm invalid format addr:%lu err:%s", fun, addr, tmp.str.c_str());
      continue;
    } else {
      const vector<RedisRv> &tmp_eles = tmp.element;
      for (int i = 0; i < (int)tmp_eles.size(); i+=2) {
        if (tmp_eles.at(i).type != REDIS_REPLY_INTEGER
            || tmp_eles.at(i+1).type != REDIS_REPLY_STRING) {
          tmp_eles.at(i).dump(log_, saddr.c_str(), 3);
          tmp_eles.at(i+1).dump(log_, saddr.c_str(), 3);
        } else {
          rvs.push_back(pair<long, string>(tmp_eles.at(i).integer, tmp_eles.at(i+1).str));
        }
      }
    }

  }


  log_->info("%s-->stamp:%d cn:%d tm:%lu rm_count:%lu", fun, stamp, count, tu.intv(), rvs.size());
  return 0;
}




int OnlineCtrl::syn(int timeout, long uid, const proto_syn &proto, proto_idx_pair &idx)
{
  //TimeUse tu;
  const char *fun = "OnlineCtrl::syn";


  string suid = boost::lexical_cast<string>(uid);

  const vector<string> &kvs = proto.data;
	size_t sz = kvs.size();
	if (sz % 2 != 0) {
		log_->error("kvs % 2 0 size:%lu", sz);
		return 1;
	}

  const script_t &script = s_syn_;
  
  vector<string> args;
  gen_redis_args(REQ_SYN, uid, &proto, args);

  args.insert(args.end(), kvs.begin(), kvs.end());


	RedisRvs rv;  
  single_uid_commend(fun, timeout, suid, args, script.data(), rv);

  return rv_check_syn(uid, rv, idx);

	//log_->info("%s-->uid:%ld tm:%ld conn:%ld sublayer:%s cli_type:%d",
  //           fun, uid, tu.intv(), proto.head.logic_conn, proto.head.sublayer_index.c_str(), proto.cli_type);


  return 0;
}

int OnlineCtrl::rv_check_syn(long uid, const RedisRvs &rv, proto_idx_pair &idx)
{
  const char *fun = "OnlineCtrl::rv_check_syn";

  if (rv.size() != 1) {
    log_->error("%s-->%ld retrun size error %lu", fun, uid, rv.size());
    return 2;
  }

  RedisRvs::const_iterator it = rv.begin();
  const RedisRv &tmp = it->second;
  //  tmp.dump(log_, fun, 0);

  if (tmp.type != REDIS_REPLY_ARRAY
      || tmp.element.size() != 2
      || tmp.element.at(0).type != REDIS_REPLY_INTEGER
      || tmp.element.at(1).type != REDIS_REPLY_INTEGER
      ) {
    tmp.dump(log_, fun, 3);
    log_->error("%s-->retrun invalid format actor:%ld err:%s", fun, uid, tmp.str.c_str());
    return 3;
  }

  idx.send_idx = tmp.element.at(0).integer;
  idx.recv_idx = tmp.element.at(1).integer;

  return 0;
}



int OnlineCtrl::gen_proto_args(int tp, const void *proto, vector<string> &args)
{

  if (REQ_SYN == tp) {
    const proto_syn *p = (const proto_syn *)proto;
    return p->keys(args);

  } else if (REQ_FIN == tp) {
    const proto_fin *p = (const proto_fin *)proto;
    return p->keys(args);

  } else if (REQ_FIN_DELAY == tp) {
    const proto_fin_delay *p = (const proto_fin_delay *)proto;
    return p->keys(args);

  } else if (REQ_UPIDX == tp) {
    const proto_upidx *p = (const proto_upidx *)proto;
    return p->keys(args);

  } else {
    return args.size();
  }

}

void OnlineCtrl::gen_redis_args(int tp, long uid, const void *proto, vector<string> &args)
{
  // tp: 0 unknown, 1 syn, 2 fin, 3 fin_delay 4 upidx
  script_t *script = NULL;
  if (REQ_SYN == tp) {
    script = &s_syn_;

  } else if (REQ_FIN == tp) {
    script = &s_fin_;

  } else if (REQ_FIN_DELAY == tp) {
    script = &s_fin_delay_;

  } else if (REQ_UPIDX == tp) {
    script = &s_upidx_;

  } else {
    return;
  }

  args.push_back("EVALSHA");
  args.push_back(script->lookup_sha1(re_));
  args.push_back("KEY_SUM");

  args.push_back(boost::lexical_cast<string>(uid));
  int key_sum = gen_proto_args(tp, proto, args);
  // update key sum
  args[2] = boost::lexical_cast<string>(key_sum - 3);

}

int OnlineCtrl::fin_delay(int timeout, long uid, const proto_fin_delay &proto)
{
  //TimeUse tu;
  const char *fun = "OnlineCtrl::fin_delay";


  string suid = boost::lexical_cast<string>(uid);

  const script_t &script = s_fin_delay_;
  
  vector<string> args;
  gen_redis_args(REQ_FIN_DELAY, uid, &proto, args);

	RedisRvs rv;  
  single_uid_commend(fun, timeout, suid, args, script.data(), rv);


  return rv_check_fin_delay(uid, rv);
}

int OnlineCtrl::rv_check_fin_delay(long uid, const RedisRvs &rv)
{
  const char *fun = "OnlineCtrl::rv_check_fin_delay";
  if (rv.size() != 1) {
    log_->error("%s-->%ld retrun size error %lu", fun, uid, rv.size());
    return 2;
  }

  RedisRvs::const_iterator it = rv.begin();
  const RedisRv &tmp = it->second;
  //  tmp.dump(log_, fun, 2);
  if (tmp.type != REDIS_REPLY_STATUS && tmp.type != REDIS_REPLY_NIL) {
    tmp.dump(log_, fun, 2);
    log_->warn("%s-->retrun invalid format actor:%ld err:%s", fun, uid, tmp.str.c_str());
  }

  if (tmp.type == REDIS_REPLY_NIL) {
    log_->warn("%s-->return actor:%ld nil", fun, uid);
  }



  //	log_->info("%s-->uid:%ld tm:%ld", fun, uid, tu.intv());
	//log_->info("%s-->uid:%ld tm:%ld conn:%ld sublayer:%s",
  //           fun, uid, tu.intv(), proto.head.logic_conn, proto.head.sublayer_index.c_str());


  return 0;

}


int OnlineCtrl::fin(int timeout, long uid, const proto_fin &proto, std::string &cli_info)
{
  //TimeUse tu;
  const char *fun = "OnlineCtrl::fin";


  string suid = boost::lexical_cast<string>(uid);

  const script_t &script = s_fin_;
  
  vector<string> args;
  gen_redis_args(REQ_FIN, uid, &proto, args);

	RedisRvs rv;  
  single_uid_commend(fun, timeout, suid, args, script.data(), rv);

  return rv_check_fin(uid, rv, cli_info);

}

int OnlineCtrl::rv_check_fin(long uid, const RedisRvs &rv, std::string &cli_info)
{
  const char *fun = "OnlineCtrl::rv_check_fin";

  if (rv.size() != 1) {
    log_->error("%s-->%ld retrun size error %lu", fun, uid, rv.size());
    return 2;
  }

  RedisRvs::const_iterator it = rv.begin();
  const RedisRv &tmp = it->second;
  //  tmp.dump(log_, fun, 2);
  int rvt = 0;
  if (tmp.type != REDIS_REPLY_STRING && tmp.type != REDIS_REPLY_NIL) {
    tmp.dump(log_, fun, 2);
    log_->warn("%s-->retrun invalid format actor:%ld err:%s", fun, uid, tmp.str.c_str());
    rvt = 2;
  } else if (tmp.type == REDIS_REPLY_NIL) {
    //log_->warn("%s-->return actor:%ld nil", fun, uid);
    rvt = 1;
  } else {
    cli_info = tmp.str;
  }

	//log_->info("%s-->uid:%ld tm:%ld conn:%ld sublayer:%s",
  //           fun, uid, tu.intv(), proto.head.logic_conn, proto.head.sublayer_index.c_str());

  return rvt;
}



int OnlineCtrl::upidx(int timeout, long uid, const proto_upidx &proto, proto_idx_pair &idx)
{
  //TimeUse tu;
  const char *fun = "OnlineCtrl::upidx";


  string suid = boost::lexical_cast<string>(uid);

  const script_t &script = s_upidx_;
  
  vector<string> args;
  gen_redis_args(REQ_UPIDX, uid, &proto, args);

	RedisRvs rv;  
  single_uid_commend(fun, timeout, suid, args, script.data(), rv);

  return rv_check_upidx(uid, rv, idx);
}

int OnlineCtrl::rv_check_upidx(long uid, const RedisRvs &rv, proto_idx_pair &idx)
{
  const char *fun = "OnlineCtrl::rv_check_upidx";
  if (rv.size() != 1) {
    log_->error("%s-->%ld retrun size error %lu", fun, uid, rv.size());
    return 2;
  }

  RedisRvs::const_iterator it = rv.begin();
  const RedisRv &tmp = it->second;
  //  tmp.dump(log_, fun, 2);

  if (tmp.type != REDIS_REPLY_ARRAY && tmp.type != REDIS_REPLY_NIL) {
    tmp.dump(log_, fun, 2);
    log_->warn("%s-->retrun invalid format actor:%ld err:%s", fun, uid, tmp.str.c_str());
    return 2;
  }

  if (tmp.type == REDIS_REPLY_NIL) {
    log_->warn("%s-->return actor:%ld nil", fun, uid);
    return 1;
  }


  if (tmp.type != REDIS_REPLY_ARRAY
      || tmp.element.size() != 2
      || tmp.element.at(0).type != REDIS_REPLY_INTEGER
      || tmp.element.at(1).type != REDIS_REPLY_INTEGER
      ) {
    tmp.dump(log_, fun, 3);
    log_->error("%s-->retrun invalid format actor:%ld err:%s", fun, uid, tmp.str.c_str());
    return 3;
  }

  idx.send_idx = tmp.element.at(0).integer;
  idx.recv_idx = tmp.element.at(1).integer;


  //	log_->info("%s-->uid:%ld tm:%ld", fun, uid, tu.intv());
	//log_->info("%s-->uid:%ld tm:%ld conn:%ld sublayer:%s cli_type:%d",
  //           fun, uid, tu.intv(), proto.head.logic_conn, proto.head.sublayer_index.c_str(), proto.cli_type);


  return 0;
}

