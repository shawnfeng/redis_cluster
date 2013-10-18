#include <boost/lexical_cast.hpp>


#include "RedisCtrl.h"
#include "Util.h"

using namespace std;

const char *CLUSTER_PREFIX = "cluster";
const char *REDIS_PREFIX = "r";


static void init_watcher(zhandle_t *zh, int type, int state, const char *path,
			 void *data)
{
	RedisCtrl *rc = (RedisCtrl *)data;
	assert(rc);
	LogOut *log = rc->log();

	log->info("init_wc-->zh=%p,type=%d,state=%d,path=%s,watcherCtx=%p",
		    zh, type, state, path, rc);
}


int RedisCtrl::start()
{
	const char *fun = "RedisCtrl::start";
  re_.start();

  // ----------------------

	zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
	zkh_ = zookeeper_init(zk_addr_.c_str(), init_watcher, 10000, 0, (void *)this, 0);
	if (!zkh_) {
		log_->error("%s-->error init zk %s zk can not used, please check add restart", fun, zk_addr_.c_str());
		return 1;
	}

	log_->info("%s-->init ok zk %s", fun, zk_addr_.c_str());

	return 0;
}

long RedisCtrl::cut_idx(const char *pref, const std::string &cluster)
{
	size_t pos = cluster.find(pref);
	if (pos == std::string::npos) {
    log_->error("cut cluster prefix %s", cluster.c_str());
    return -1;
	}

	long idx = -1;
  string sidx = cluster.substr(pos+strlen(pref));
  try {
    
    idx = boost::lexical_cast<long>(sidx);
  } catch(std::exception& e) {
    log_->error("cast idx error %s %s err:%s", cluster.c_str(), sidx.c_str(), e.what());
  }

  return idx;
}

bool RedisCtrl::isvalid_addr(const char *addr)
{
  string err;
  string ip;
  int port;
  return str_ipv4(addr, ip, port, err);

}

int RedisCtrl::get_nodes(const char *path, bool isaddr, set<string> &addrs)
{
  const char *fun = "RedisCtrl::get_nodes";
  struct String_vector node_info;
  int rc = zoo_get_children(zkh_, path, 0, &node_info);
  if (rc == ZOK) {
		char **dp = node_info.data;
		for (int i = 0; i < node_info.count; ++i) {
			char *chd = *dp++;
			log_->debug("%s-->%s/%s", fun, path, chd);

      if (isaddr) {
        if (isvalid_addr(chd)) {
          addrs.insert(chd);
        } else {
          log_->error("%s-->invalid addr %s/%s", fun, path, chd);
        }
      } else {
        addrs.insert(chd);
      }
		}


    deallocate_String_vector(&node_info);
  } else {
    log_->error("%s-->rc:%d get path %s error", fun, rc, path);
    return 1;
  }

  log_->info("%s-->path:%s addrs.size:%lu", fun, path, addrs.size());
  return 0;


}

int RedisCtrl::get_cluster_node(const char *path, bool ischeck, map< string, set<string> > &cfgs)
{
  const char *fun = "RedisCtrl::get_cluster_node";

  struct String_vector cfg_info;
  int rc = zoo_get_children(zkh_, path, 0, &cfg_info);

  set<string> clusters;
  if (rc == ZOK) {
		char **dp = cfg_info.data;
		for (int i = 0; i < cfg_info.count; ++i) {
			char *chd = *dp++;
			log_->debug("%s-->%s/%s", fun, path, chd);
      clusters.insert(chd);
		}

    deallocate_String_vector(&cfg_info);
  } else {
    log_->error("%s-->zoo_get_children get %d %s error ", fun, rc, path);
    return 1;
  }

  // ----------
  for (set<string>::const_iterator it = clusters.begin();
       it != clusters.end(); ++it) {
    if (ischeck) {
      long idx = cut_idx(CLUSTER_PREFIX, *it);
      if (-1 == idx) continue;
    }

    set<string> &nodes = cfgs.insert(pair< string , set<string> >(*it, set<string>())).first->second;
    string tmp = path;
    tmp += "/";
    tmp += *it;
    if (get_nodes(tmp.c_str(), ischeck, nodes)) return 2;

  }
  return 0;
}

int RedisCtrl::get_config(const char *path, std::map< string, std::set<std::string> > &cfgs)
{
  return get_cluster_node(path, true, cfgs);

}


int RedisCtrl::get_error(const char *path, std::map< string, std::set<std::string> > &errs)
{
  return get_cluster_node(path, false, errs);
}

int RedisCtrl::create_node(const char *path, int flags, const string &value, string &true_path)
{
  const char *fun = "RedisCtrl::create_node";
  const char *v = NULL;
  int vl = -1;
  if (!value.empty()) {
    v = value.c_str();
    vl = (int)value.size();
  }

  char buf[1024];

  int rv = zoo_create(zkh_, path, v, vl,
                      &ZOO_OPEN_ACL_UNSAFE, flags,
                      buf, sizeof(buf));

  if (rv != ZOK) {
    log_->error("%s create node err path:%s flags:%d value%s rv:%d",
                fun, path, flags, value.c_str(), rv
                );
    return 1;
  }

  true_path = buf;
  log_->info("%s-->create zk node path:%s truepath:%s flags:%d value:%s",
             fun, path, true_path.c_str(), flags, value.c_str()
             );
  return 0;

}

int RedisCtrl::delete_node(const char *path)
{
  const char *fun = "RedisCtrl::delete_node";

  int rv = zoo_delete(zkh_, path, -1);

  if (rv == ZOK) {
    log_->info("%s delete_node path:%s",
                fun, path
                );

    return 0;
  } else {
    log_->error("%s delete_node err path:%s rv:%d",
                fun, path, rv
                );

    return 1;
  }


}

int RedisCtrl::node_exist(const char *path)
{
  const char *fun = "RedisCtrl::is_node_exist";
  struct Stat stat;
  int rv = zoo_exists(zkh_, path, 0, &stat);

  log_->trace("%s-->path:%s "
              "stat "
              "czxid:%ld mzxid:%ld ctime:%ld mtime:%ld "
              "version:%ld cversion:%ld aversion:%ld "
              "ephemeralOwner:%ld dataLength:%ld numChildren:%ld pzxid:%ld",
              fun, path,
              stat.czxid, stat.mzxid, stat.ctime, stat.mtime,
              stat.version, stat.cversion, stat.aversion,
              stat.ephemeralOwner, stat.dataLength, stat.numChildren, stat.pzxid
              );

  if (rv == ZOK) return 0;
  else if (rv == ZNONODE) return 1;
  else {
    log_->error("%s node_exist err path:%s rv:%d",
                fun, path, rv
                );

    return 2;
  }
}

int RedisCtrl::get_data(const char *path, string &data)
{
  const char *fun = "RedisCtrl::get_data";
  char buf[256];
  int buf_len = (int)sizeof(buf);
  struct Stat stat;
  int rv = zoo_get(zkh_, path, 0, buf, &buf_len, &stat);
  if (ZOK != rv) {
    log_->error("%s-->zoo_get error rv:%d path:%s", fun, rv, path);
    return 1;
  }

  data.assign(buf, buf_len);
  /*
    int64_t czxid;
    int64_t mzxid;
    int64_t ctime;
    int64_t mtime;
    int32_t version;
    int32_t cversion;
    int32_t aversion;
    int64_t ephemeralOwner;
    int32_t dataLength;
    int32_t numChildren;
    int64_t pzxid;
  */
  log_->trace("%s-->path:%s data:%s",
              fun, path, data.c_str());

  log_->trace("%s-->path:%s "
              "stat "
              "czxid:%ld mzxid:%ld ctime:%ld mtime:%ld "
              "version:%ld cversion:%ld aversion:%ld "
              "ephemeralOwner:%ld dataLength:%ld numChildren:%ld pzxid:%ld",
              fun, path,
              stat.czxid, stat.mzxid, stat.ctime, stat.mtime,
              stat.version, stat.cversion, stat.aversion,
              stat.ephemeralOwner, stat.dataLength, stat.numChildren, stat.pzxid
              );

  return 0;
}

int RedisCtrl::get_check(const char *path, map< string, map<string, string> > &chks)
{
  const char *fun = "RedisCtrl::get_check";
  map< string, set<string> > cls;
  if (get_cluster_node(path, false, cls)) return 1;
  

  char buf[1024];
  for (map< string, set<string> >::const_iterator it = cls.begin();
       it != cls.end(); ++it) {
    map<string, string> &rdis
      = chks.insert(pair< string, map<string, string> >(it->first, map<string, string>())).first->second;

    for (set<string>::const_iterator jt = it->second.begin();
         jt != it->second.end(); ++jt) {
      snprintf(buf, sizeof(buf), "%s/%s/%s",
               path, it->first.c_str(), jt->c_str());

      string data;
      if (get_data(buf, data)) {
        return 2;
      }

      long idx = cut_idx(REDIS_PREFIX, *jt);
      if (idx == -1) {
        if (delete_node(buf)) return 3;      
        continue;
      }

      map<string, string>::const_iterator kt = rdis.find(data);
      if (kt != rdis.end()) {
        log_->error("%s-->multi addr %s %s", fun, data.c_str(), jt->c_str());
        
        long idx0 = cut_idx(REDIS_PREFIX, kt->second);
        long idx1 = cut_idx(REDIS_PREFIX, *jt);

        if (idx1 < idx0) {
          rdis[data] = *jt;
          snprintf(buf, sizeof(buf), "%s/%s/%s",
                   path, it->first.c_str(), kt->second.c_str());
        }

        if (delete_node(buf)) return 4;
        
      } else {
        rdis[data] = *jt;
      }
    }

  }

  return 0;
}

int RedisCtrl::init_path_check()
{
  string true_path;

  if (check_add_create(zk_lock_path_.c_str(), 0, "", true_path))
    return 1;

  if (check_add_create(zk_cfg_path_.c_str(), 0, "", true_path))
    return 2;

  if (check_add_create(zk_err_path_.c_str(), 0, "", true_path))
    return 3;

  if (check_add_create(zk_check_path_.c_str(), 0, "", true_path))
    return 4;

  if (check_add_create(zk_node_path_.c_str(), 0, "", true_path))
    return 5;


  return 0;
}

void RedisCtrl::check()
{
  const char *fun = "RedisCtrl::check";

  log_->info("%s-->check zk %p", fun, zkh_);
  if (!zkh_) {
    log_->warn("%s-->zkhandle NULL", fun);
    return;
  }

  log_->info("%s-->init check", fun);
  int rv = init_path_check();
  if (rv) {
    log_->error("%s-->init check path error rv:%d", fun, rv);
    return;
  }

  // !!! 先check 给的路径是否存在，不存在创建之
  // 获取锁

  //-----------------------------------

  map< string, set<string> > cfgs;

  rv = get_config(zk_cfg_path_.c_str(), cfgs);
  log_->info("%s-->get config rv:%d size:%lu", fun, rv, cfgs.size());
  if (rv != 0) {
    log_->error("%s-->get cfg error rv:%d", fun, rv);
    return;
  }


  log_->debug("%s-->rv:%d cluster.size:%lu",
              fun, rv, cfgs.size());

  for (map< string, set<string> >::const_iterator it = cfgs.begin();
       it != cfgs.end(); ++it) {
    for (set<string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
      log_->info("%s-->configinfo %s node:%s", fun, it->first.c_str(), jt->c_str());
    }

  }
  // -----------------------------------------
  map< string, set<string> > errs;
  rv = get_error(zk_err_path_.c_str(), errs);
  log_->info("%s-->get error rv:%d size:%lu", fun, rv, errs.size());
  if (rv != 0) {
    log_->error("%s-->get errs error rv:%d", fun, rv);
    return;
  }
  log_->debug("%s-->rv:%d errs.size:%lu",
              fun, rv, errs.size());

  for (map< string, set<string> >::const_iterator it = errs.begin();
       it != errs.end(); ++it) {
    for (set<string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
      log_->info("%s-->errors %s node:%s", fun, it->first.c_str(), jt->c_str());
    }

  }


  // -----------------------------------------
  map< string, map<string, string> > chks;
  rv = get_check(zk_check_path_.c_str(), chks);
  log_->info("%s-->get check rv:%d size:%lu", fun, rv, chks.size());
  if (rv != 0) {
    log_->error("%s-->get checks error rv:%d", fun, rv);
    return;
  }
  log_->debug("%s-->rv:%d chks.size:%lu",
              fun, rv, chks.size());

  for (map< string, map<string, string> >::const_iterator it = chks.begin(); it != chks.end(); ++it) {
  for (map<string, string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
    log_->info("%s-->checks %s node:%s %s", fun, it->first.c_str(), jt->first.c_str(), jt->second.c_str());
    }

  }

 
  // begin check -----------------------------------------
  // add new check
 rv = logic_check_add(cfgs, errs, chks); 
 log_->info("%s-->logic_check_add rv:%d", fun, rv);
 if (rv != 0) {
   log_->error("%s-->get logic_check_add error rv:%d", fun, rv);
   return;
 }

 // error remove check
 rv = logic_error_rm(cfgs, errs);
 log_->info("%s-->logic_error_rm rv:%d", fun, rv);
 if (rv != 0) {
   log_->error("%s-->get logic_error_rm error rv:%d", fun, rv);
   return;
 }

 // check remove
 rv = logic_check_rm(cfgs, chks);
 log_->info("%s-->logic_check_rm rv:%d", fun, rv);
 if (rv != 0) {
   log_->error("%s-->get logic_check_rm error rv:%d", fun, rv);
   return;
 }


 // check && get redis config
 set<string> addrs;
 for (map< string, set<string> >::const_iterator it = cfgs.begin();
      it != cfgs.end(); ++it) {
   addrs.insert(it->second.begin(), it->second.end());
 }

 map<string, string> redis_cfgs;
 check_redis(addrs,  redis_cfgs);
 log_->info("%s-->check_redis size:%lu", fun, redis_cfgs.size());
 for (map<string, string>::const_iterator it = redis_cfgs.begin(); it != redis_cfgs.end(); ++it) {
   log_->info("%s-->check_redis addr:%s slaveof:%s", fun, it->first.c_str(), it->second.c_str());
 }

 // error check
 check_error(redis_cfgs);
 log_->info("%s-->check_error", fun);
 // normal check

}

int RedisCtrl::check_add_create(const char *path, int flags, const std::string &value, std::string &true_path)
{
  int rv = node_exist(path);

  if (rv == 1) {
    rv = create_node(path, flags, value, true_path);
    if (rv) return 1;
    else return node_exist(true_path.c_str());
  } else if (rv == 2) {
    return 3;
  } else  {
    assert(rv == 0);
    return 0;
  }

}

int RedisCtrl::logic_error_rm(const std::map< std::string, std::set<std::string> > &cfgs,
                              const std::map< std::string, std::set<std::string> > &errs)
{
  const char *fun = "RedisCtrl::logic_error_rm";
  for (map< string, set<string> >::const_iterator it = errs.begin();
       it != errs.end(); ++it) {

    string path = zk_err_path_ + "/";
    path += it->first;
    

    map< string, set<string> >::const_iterator tmp_cfg = cfgs.find(it->first);

    for (set<string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
      if (tmp_cfg == cfgs.end()) {
        log_->warn("%s-->can not find cluster config %s rm node %s", fun, it->first.c_str(), jt->c_str());
        if (delete_node((path + "/" + *jt).c_str())) return 2;

      } else if (tmp_cfg->second.find(*jt) == tmp_cfg->second.end()) {
        log_->warn("%s-->can not find node config %s/%s", fun, it->first.c_str(),jt->c_str());
        if (delete_node((path + "/" + *jt).c_str())) return 2;

      }
    }

    if (tmp_cfg == cfgs.end()) {
        log_->warn("%s-->can not find cluster config %s rm cluster", fun, it->first.c_str());
      if (delete_node(path.c_str())) return 1;

    }


  }

  return 0;
}

int RedisCtrl::logic_check_rm(const std::map< std::string, std::set<std::string> > &cfgs,
                              const std::map< std::string, std::map<std::string, string> > &chks)
{
  for (map< string, map<string, string> >::const_iterator it = chks.begin();
       it != chks.end(); ++it) {

    string path = zk_check_path_ + "/";
    path += it->first;
    

    map< string, set<string> >::const_iterator tmp_cfg = cfgs.find(it->first);

    for (map<string, string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
      char buf[1024];
      snprintf(buf, sizeof(buf), "%s/%s", path.c_str(), jt->second.c_str());

      if (tmp_cfg == cfgs.end()) {
        if (delete_node(buf)) return 2;

      } else if (tmp_cfg->second.find(jt->first) == tmp_cfg->second.end()) {
        if (delete_node(buf)) return 2;

      }
    }

    if (tmp_cfg == cfgs.end()) {
      if (delete_node(path.c_str())) return 1;

    }


  }

  return 0;
}


int RedisCtrl::logic_check_add(const std::map< std::string, std::set<std::string> > &cfgs,
                               const std::map< std::string, std::set<std::string> > &errs,
                               const std::map< std::string, std::map<std::string, string> > &chks)
{
  for (map< string, set<string> >::const_iterator it = cfgs.begin();
       it != cfgs.end(); ++it) {

    string path = zk_check_path_ + "/";
    path += it->first;
    string redis_path = path + "/";
    redis_path += REDIS_PREFIX;
    string true_path;
    
    

    map< string, map<string, string> >::const_iterator tmp = chks.find(it->first);
    map< string, set<string> >::const_iterator tmp_err = errs.find(it->first);

    if (tmp == chks.end()) {
      if (create_node(path.c_str(), 0, "", true_path)) {
        return 1;
      }
    }

    for (set<string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
      if (tmp != chks.end() && tmp->second.find(*jt) != tmp->second.end()) {
        continue;
      }


      if (tmp_err != errs.end() && tmp_err->second.find(*jt) != tmp_err->second.end()) {
        continue;
      }


      if (create_node(redis_path.c_str(), ZOO_SEQUENCE, *jt, true_path)) {
        return 2;
      }
    }


  }

  return 0;
}

void RedisCtrl::check_redis(const set<string> &addrs, map<string, string> &cfgs, int try_times)
{
  const char *fun = "RedisCtrl::check_redis";

  RedisRvs rv;
  map< uint64_t, vector<string> > addr_cmd;
  

  for (set<string>::const_iterator it = addrs.begin();
       it != addrs.end(); ++it) {
    uint64_t addr;
    string err;

    if (!str_ipv4_int64(*it, addr, err)) {
      log_->error("%s-->error addr:%s err:%s", fun, it->c_str(), err.c_str());
    } else {
      addr_cmd[addr] = vector<string>();
    }
  }
  // CONFIG GET commend
  for (map< uint64_t, vector<string> >::iterator it = addr_cmd.begin();
       it != addr_cmd.end(); ++it) {

    it->second.push_back("CONFIG");
    it->second.push_back("GET");
    it->second.push_back("slaveof");

  }

  re_.cmd(rv, "check_redis", addr_cmd, 100, "", false);


	for (RedisRvs::const_iterator it = rv.begin(); it != rv.end(); ++it) {
    string addr;
    int64_str_ipv4(it->first, addr);
    it->second.dump(log_, addr.c_str(), 1);
    const RedisRv &tmp = it->second;

    if (tmp.type != REDIS_REPLY_ARRAY
        || tmp.element.size() != 2
        || tmp.element.at(0).type != REDIS_REPLY_STRING
        || tmp.element.at(1).type != REDIS_REPLY_STRING
        ) {

      log_->error("%s-->retrun config invalid format addr:%s", fun, addr.c_str());
      continue;

    } else {
      if (tmp.element.at(0).str != "slaveof") {
        log_->error("%s-->retrun config invalid back:%s addr:%s", fun, tmp.element.at(0).str.c_str(), addr.c_str());
        continue;

      } else {
        cfgs[addr] = tmp.element.at(1).str;
      }

    }


	}

  set<string> err_addrs;
  for (set<string>::const_iterator it = addrs.begin();
       it != addrs.end(); ++it) {
    if (cfgs.find(*it) == cfgs.end()) {
      err_addrs.insert(*it);
    }
  }


  log_->info("%s-->addrs.size:%lu erraddrs.size:%lu cfgs:%lu try_times:%d",
             fun, addrs.size(), err_addrs.size(), cfgs.size(), try_times);

  if (!err_addrs.empty() && --try_times) {
    sleep(1); // wait check
    check_redis(err_addrs, cfgs, try_times);
  }

}

void RedisCtrl::check_error(const map<string, string> &cfgs)
{
  const char *fun = "RedisCtrl::check_error";

  string true_path;
  char path_buff[1024];


  map< string, set<string> > errs;
  int rv = get_error(zk_err_path_.c_str(), errs);
  if (rv != 0) {
    log_->error("%s-->get errs error rv:%d", fun, rv);
    return;
  }


  for (map< string, set<string> >::const_iterator it = errs.begin();
       it != errs.end(); ++it) {
    for (set<string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
      log_->trace("%s-->check %s node:%s", fun, it->first.c_str(), jt->c_str());

      if (cfgs.find(*jt) != cfgs.end()) {
        log_->info("%s-->repair %s node:%s", fun, it->first.c_str(), jt->c_str());
        // node repair
        snprintf(path_buff, sizeof(path_buff), "%s/%s/%s",
                 zk_err_path_.c_str(),
                 it->first.c_str(),
                 jt->c_str()
                 );

        if (delete_node(path_buff)) {
          return;
        }
        
        snprintf(path_buff, sizeof(path_buff), "%s/%s/%s",
                 zk_check_path_.c_str(),
                 it->first.c_str(),
                 REDIS_PREFIX
                 );
        if (create_node(path_buff, ZOO_SEQUENCE, *jt, true_path)) {
          return;
        }
      }
      
    }

  }


}
