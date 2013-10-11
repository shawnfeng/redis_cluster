#ifndef __REDIS_HASH_H_H__
#define __REDIS_HASH_H_H__

#include <vector>
#include <map>
#include <string>

#include <boost/thread/mutex.hpp>

#include <zookeeper/zookeeper.h>

#include "Util.h"
#include "RedisEvent.h"

struct zk_ctx_t {
	std::string addr;
	zhandle_t *h;
	std::string path;


zk_ctx_t(const char *zk_addr,
	 const char *zk_path) :
	addr(zk_addr), h(NULL), path(zk_path) {}
};


class RedisHash {
	LogOut *log_;
	RedisEvent *re_;
	boost::shared_mutex smux_;
	std::set<uint64_t> addrs_;
  std::vector<uint64_t> addrs_v_;

	// zookeeper
	zk_ctx_t zk_ctx_;

 public:

 RedisHash(LogOut *log, RedisEvent *re,
	   const char *zk_addr,
	   const char *zk_path)
	 : log_(log), re_(re),
		zk_ctx_(zk_addr, zk_path)
		{

		}
	~RedisHash() { if (zk_ctx_.h) zookeeper_close(zk_ctx_.h);}
	void update_ends(const std::vector< std::pair<std::string, int> > &ends);
	zk_ctx_t *zk_ctx() { return &zk_ctx_; }
	int start();

  uint64_t redis_addr(const std::string &key);

  //	void hash_addr(const std::vector<std::string> &hash, std::set<uint64_t> &addrs);
	LogOut *log() { return log_; }


};

#endif
