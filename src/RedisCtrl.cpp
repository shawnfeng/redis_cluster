#include <boost/lexical_cast.hpp>
#include "RedisCtrl.h"

using namespace std;

const char *LOCK_PATH = "/lock";
const char *CONFIG_PATH = "/config";
const char *ERROR_PATH = "/error";
const char *CHECK_PATH = "/check";

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

int RedisCtrl::cut_idx(const char *pref, const std::string &cluster)
{
	size_t pos = cluster.find(pref);
	if (pos == std::string::npos) {
    log_->error("cut cluster prefix %s", cluster.c_str());
    return -1;
	}

	int idx = -1;
  string sidx = cluster.substr(pos+strlen(pref));
  try {
    
    idx = boost::lexical_cast<int>(sidx);
  } catch(std::exception& e) {
    log_->error("cast idx error %s %s", cluster.c_str(), sidx.c_str());
  }

  return idx;
}

void RedisCtrl::get_nodes(const char *path, set<string> &addrs)
{
  const char *fun = "RedisCtrl::get_nodes";
  struct String_vector node_info;
  int rc = zoo_get_children(zkh_, path, 0, &node_info);
  if (rc == ZOK) {
		char **dp = node_info.data;
		for (int i = 0; i < node_info.count; ++i) {
			char *chd = *dp++;
			log_->debug("%s-->%s/%s", fun, path, chd);
      addrs.insert(chd);
		}


    deallocate_String_vector(&node_info);
  } else {
    log_->error("%s-->rc:%d get path %s error", fun, rc, path);
  }

  log_->info("%s-->path:%s addrs.size:%lu", fun, path, addrs.size());

}

int RedisCtrl::get_cluster_node(const char *path, map< string, set<string> > &cfgs)
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
    int idx = cut_idx(CLUSTER_PREFIX, *it);
    if (-1 == idx) continue;

    set<string> &nodes = cfgs.insert(pair< string , set<string> >(*it, set<string>())).first->second;
    string tmp = path;
    tmp += "/";
    tmp += *it;
    get_nodes(tmp.c_str(), nodes);

  }
  return 0;
}

int RedisCtrl::get_config(const char *path, std::map< string, std::set<std::string> > &cfgs)
{
  return get_cluster_node(path, cfgs);

}


int RedisCtrl::get_error(const char *path, std::map< string, std::set<std::string> > &errs)
{
  return get_cluster_node(path, errs);
}

int RedisCtrl::create_node(const char *path, int flags, const string &value, string &true_path)
{
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

  if (rv != ZOK) return 1;

  return 0;

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

int RedisCtrl::get_check(const char *path, map< string, map<string, int> > &chks)
{
  map< string, set<string> > cls;
  if (get_cluster_node(path, cls)) return 1;
  

  char buf[1024];
  for (map< string, set<string> >::const_iterator it = cls.begin();
       it != cls.end(); ++it) {
    map<string, int> &rdis
      = chks.insert(pair< string, map<string, int> >(it->first, map<string, int>())).first->second;

    for (set<string>::const_iterator jt = it->second.begin();
         jt != it->second.end(); ++jt) {
      snprintf(buf, sizeof(buf), "%s/%s/%s",
               path, it->first.c_str(), jt->c_str());

      string data;
      if (get_data(buf, data)) {
        return 2;
      }

      int idx = cut_idx(REDIS_PREFIX, *jt);
      if (idx == -1) continue;

      rdis[data] = idx;
    }

  }

  return 0;
}


void RedisCtrl::check()
{
  const char *fun = "RedisCtrl::check";

  if (!zkh_) {
    log_->warn("%s-->zkhandle NULL", fun);
    return;
  }

  // !!! 先check 给的路径是否存在，不存在创建之
  // 获取锁

  string lock_p = zk_root_path_ + LOCK_PATH;
  string cfg_p = zk_root_path_ + CONFIG_PATH;
  string err_p = zk_root_path_ + ERROR_PATH;
  string check_p = zk_root_path_ + CHECK_PATH;
  //-----------------------------------

  map< string, set<string> > cfgs;

  int rv = get_config(cfg_p.c_str(), cfgs);
  if (rv != 0) {
    log_->error("%s-->get cfg error rv:%d", fun, rv);
    return;
  }


  log_->debug("%s-->rv:%d cluster.size:%lu",
              fun, rv, cfgs.size());

  for (map< string, set<string> >::const_iterator it = cfgs.begin();
       it != cfgs.end(); ++it) {
    for (set<string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
      log_->trace("%s-->configinfo %s node:%s", fun, it->first.c_str(), jt->c_str());
    }

  }
  // -----------------------------------------
  map< string, set<string> > errs;
  rv = get_error(err_p.c_str(), errs);
  if (rv != 0) {
    log_->error("%s-->get errs error rv:%d", fun, rv);
    return;
  }
  log_->debug("%s-->rv:%d errs.size:%lu",
              fun, rv, errs.size());

  for (map< string, set<string> >::const_iterator it = errs.begin();
       it != errs.end(); ++it) {
    for (set<string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
      log_->trace("%s-->errors %s node:%s", fun, it->first.c_str(), jt->c_str());
    }

  }


  // -----------------------------------------
  map< string, map<string, int> > chks;
  rv = get_check(check_p.c_str(), chks);
  if (rv != 0) {
    log_->error("%s-->get checks error rv:%d", fun, rv);
    return;
  }
  log_->debug("%s-->rv:%d chks.size:%lu",
              fun, rv, chks.size());

  for (map< string, map<string, int> >::const_iterator it = chks.begin(); it != chks.end(); ++it) {
  for (map<string, int>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
    log_->trace("%s-->checks %s node:%s r%d", fun, it->first.c_str(), jt->first.c_str(), jt->second);
    }

  }

 
  // begin check -----------------------------------------
  // add new check

 if (!logic_check_add(cfgs, errs, chks)) return;



}

int RedisCtrl::logic_check_add(const std::map< std::string, std::set<std::string> > &cfgs,
                               const std::map< std::string, std::set<std::string> > &errs,
                               const std::map< std::string, std::map<std::string, int> > &chks)
{
  for (map< string, set<string> >::const_iterator it = cfgs.begin();
       it != cfgs.end(); ++it) {
    for (set<string>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {

    }

  }

  return 0;
}
