#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "city.h"

#include "RedisHash.h"

using namespace std;


static void redis_node_wc(zhandle_t *zh, int type, int state, const char *path, void *data);
static void redis_node_cb(int rc, const struct String_vector *strings, const void *data);
static void init_watcher(zhandle_t *zh, int type, int state, const char *path, void *data);

static void init_watcher(zhandle_t *zh, int type, int state, const char *path,
			 void *data)
{
	RedisHash *rh = (RedisHash *)data;
	assert(rh);
	LogOut *log = rh->log();

	log->info("init_wc-->zh=%p,type=%d,state=%d,path=%s,watcherCtx=%p",
		    zh, type, state, path, rh);
}

static void redis_node_wc(zhandle_t *zh, int type, int state, const char *path, void *data)
{
	const char *fun = "redis_node_wc";
	RedisHash *rh = (RedisHash *)data;
	assert(rh);
	LogOut *log = rh->log();

	log->info("%s-->zh=%p,type=%d,state=%d,path=%s,watcherCtx=%p",
		  fun, zh, type, state, path, data);


	log->info("%s-->readd watcher", fun);
	zoo_awget_children(zh,
			   path,
			   redis_node_wc, data,
			   redis_node_cb, data);


}

static int path_split(const char *path, int &cls, vector< pair<string, int> > &sp, string &err)
{
  vector<string> tmp;
  boost::algorithm::split(tmp, path, boost::algorithm::is_any_of(";"));

  if (tmp.size() < 2) {
    err = "error path size";
    return 1;
  }

  try {
    cls = boost::lexical_cast<int>(tmp.at(0));
  } catch(std::exception& e) {
    err = "error cluster index";
    return 2;
  }

  for (size_t i = 1; i < tmp.size(); ++i) {
    string ip;
    int port;

    if (!str_ipv4(tmp.at(i), ip, port, err)) {
      err = "addr format error";
      return 3;
    } else {
      sp.push_back(pair<string, int>(ip, port));
    }

  }

  return 0;

}


static void redis_node_cb(int rc, const struct String_vector *strings, const void *data)
{

	const char *fun = "redis_node_cb";
	RedisHash *rh = (RedisHash *)data;
	assert(rh);
	LogOut *log = rh->log();
	zk_ctx_t *ctx = rh->zk_ctx();

	log->warn("%s-->rc=%d,strings=%p,data=%p",
		  fun, rc, strings, data
		  );

	if (ZOK != rc) {
		// 节点不存在时候，后续即使节点被创建了也不会触发watcher
		// 所以要再设置一下
		log->error("%s-->error rc=%d rewatch", fun, rc);
		zoo_awget_children(ctx->h,
				   ctx->path.c_str(),
				   redis_node_wc, (void *)data,
				   redis_node_cb, data);

		sleep(1);

	} else {

		char **dp = strings->data;
		map< int, vector< pair<string, int> > > ends;

		for (int i = 0; i < strings->count; ++i) {
			char *chd = *dp++;
			log->info("%s-->%s/%s", fun, ctx->path.c_str(), chd);

			string err;
      int cls;
      vector< pair<string, int> > vrds;
      if (path_split(chd, cls, vrds, err)) {
        log->error("%s-->child %s format error: %s", fun, chd, err.c_str());
        return;
      }
      ends[cls] = vrds;

		}

		rh->update_ends(ends);

	}


}

int RedisHash::start()
{
	const char *fun = "RedisHash::start";
	zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
	zhandle_t *zkh = zookeeper_init(zk_ctx_.addr.c_str(), init_watcher, 10000, 0, (void *)this, 0);
	if (!zkh) {
		log_->error("%s-->error init zk %s zk can not used, please check add restart", fun, zk_ctx_.addr.c_str());
		return 1;
	}

	zk_ctx_.h = zkh;
	log_->info("%s-->init ok zk %s", fun, zk_ctx_.addr.c_str());

	int rc = zoo_awget_children(zkh,
				    zk_ctx_.path.c_str(),
				    redis_node_wc, (void *)this,
				    redis_node_cb, (void *)this);


	if (rc != ZOK) {
		log_->error("zk awget children error %d!", rc);
		return 2;
	}

	return 0;
}

static bool sort_cluster_fun(const pair< int, vector<uint64_t> > &a, const pair< int, vector<uint64_t> > &b)
{
  return a.second < b.second;
}


void RedisHash::update_ends(const map< int, vector< pair<string, int> > > &ends)
{

	const char *fun = "RedisHash::update_ends";
	log_->info("%s-->size:%lu", fun, ends.size());

  typedef map< int, vector< pair<string, int> > > addrm_t;


  map< int, vector<uint64_t> > maddrs;
	for (addrm_t::const_iterator it = ends.begin();
		     it != ends.end();
		     ++it) {
    vector<uint64_t> &addrs = maddrs.insert(
                                            pair< int, vector<uint64_t> >(it->first, vector<uint64_t>())
                                              ).first->second;

    for (vector< pair<string, int> >::const_iterator jt = it->second.begin();
         jt != it->second.end(); ++jt) {
      uint64_t addr = ipv4_int64(jt->first.c_str(), jt->second);
      log_->info("%s-->cluster%d ends:%s:%d addr:%lu", fun, it->first, jt->first.c_str(), jt->second, addr);
      addrs.push_back(addr);
    }

	}


	boost::unique_lock< boost::shared_mutex > lock(smux_);
	addrs_.swap(maddrs);
  addrs_v_.clear();
  addrs_v_.assign(addrs_.begin(), addrs_.end());
  sort(addrs_v_.begin(), addrs_v_.end(), sort_cluster_fun);

}

void RedisHash::redis_all(map<int, uint64_t> &addrs)
{
  const char *fun = "RedisHash::redis_all";
	boost::shared_lock< boost::shared_mutex > lock(smux_);
  //  addrs = addrs_;
  typedef vector<
    std::pair< int, std::vector<uint64_t> >
    > addrv_t;
  for (addrv_t::const_iterator it = addrs_v_.begin(); it != addrs_v_.end(); ++it) {
    if (!it->second.empty()) {
      addrs[it->first] = it->second[0];
    } else {
      log_->error("%s-->redis addrs emtpy!", fun);
    }
  }
}

uint64_t RedisHash::redis_addr_slave_first(const std::string &key)
{
	const char *fun = "redis_addr_slave_first";
	boost::shared_lock< boost::shared_mutex > lock(smux_);
  if (addrs_.empty() || addrs_v_.empty()) {
    log_->error("%s-->redis cluster emtpy!", fun);
    return 0;
  }

  uint64_t tmp = CityHash64(key.c_str(), key.size());
  log_->trace("%s-->hash:%s city64:%lu", fun, key.c_str(), tmp);

  const vector<uint64_t> &rds = addrs_v_.at(tmp % addrs_v_.size()).second;
  if (rds.empty()) {
    log_->error("%s-->redis addrs emtpy!", fun);
    return 0;

  }

  int cn_sv = (int)rds.size()-1;
  int idx = 0;
  if (cn_sv > 0) {
    idx = rand() % cn_sv + 1;
  }

  tmp = rds.at(idx);
  return tmp;

}

uint64_t RedisHash::redis_addr_master(const std::string &key)
{
	const char *fun = "redis_addr_master";

	boost::shared_lock< boost::shared_mutex > lock(smux_);
  if (addrs_.empty() || addrs_v_.empty()) {
    log_->error("%s-->redis cluster emtpy!", fun);
    return 0;
  }

  uint64_t tmp = CityHash64(key.c_str(), key.size());
  log_->trace("%s-->hash:%s city64:%lu", fun, key.c_str(), tmp);

  const vector<uint64_t> &rds = addrs_v_.at(tmp % addrs_v_.size()).second;
  if (rds.empty()) {
    log_->error("%s-->redis addrs emtpy!", fun);
    return 0;

  }

  tmp = rds.at(0);

  return tmp;
}

/*
void RedisHash::hash_addr(const vector<string> &hash, std::set<uint64_t> &addrs)
{
	const char *fun = "RedisHash::hash_addr";


	boost::shared_lock< boost::shared_mutex > lock(smux_);

  if (addrs_.empty() || addrs_v_.empty()) {
    log_->error("%s-->redis addrs emtpy!", fun);
    return;
  }

  uint64 tmp;
  for (vector<string>::const_iterator it = hash.begin(); it != hash.end(); ++it) {
    tmp = redis_addr(*it);
    if (tmp) addrs.insert(addrs_v_.at(tmp % addrs_v_.size()));
  }

  // !!!!!!!!!!!
  addrs = addrs_;

	log_->debug("%s-->hash:%lu addrs:%lu", fun, hash.size(), addrs.size());
}
*/

