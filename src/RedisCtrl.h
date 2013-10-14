#ifndef __REDIS_CTRL_H_H__
#define __REDIS_CTRL_H_H__

#include <vector>
#include <map>
#include <string>

#include <boost/thread/mutex.hpp>

#include <zookeeper/zookeeper.h>

#include "Util.h"
#include "RedisEvent.h"

class RedisCtrl {
	LogOut *log_;
	RedisEvent re_;
	// zookeeper
	std::string zk_addr_;
  std::string zk_root_path_;
	zhandle_t *zkh_;


  // ---------
  void get_nodes(const char *path, std::set<std::string> &addrs);
  int cut_idx(const char *pref, const std::string &cluster);

  int get_data(const char *path, std::string &data);
  int create_node(const char *path, int flags, const std::string &value, std::string &true_path);

  int get_cluster_node(const char *path, std::map< int, std::set<std::string> > &cfgs);

  int get_config(const char *path, std::map< int, std::set<std::string> > &cfgs);
  int get_error(const char *path, std::map< int, std::set<std::string> > &errs);
  int get_check(const char *path, std::map< int, std::map<std::string, int> > &chks);

  // check add logic
  int logic_check_add(const std::map< int, std::set<std::string> > &cfgs,
                      const std::map< int, std::set<std::string> > &errs,
                      const std::map< int, std::map<std::string, int> > &chks);

 public:

 RedisCtrl(LogOut *log,
	   const char *zk_addr,
	   const char *zk_root_path)
	 : log_(log), re_(log), zk_addr_(zk_addr), zk_root_path_(zk_root_path), zkh_(NULL)
		{

		}
	LogOut *log() { return log_; }
  int start();

  void check();
};

#endif
