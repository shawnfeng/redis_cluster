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

  std::string zk_lock_path_;
  std::string zk_cfg_path_;
  std::string zk_err_path_;
  std::string zk_check_path_;

  // ---------
  bool isvalid_addr(const char *addr);

  int get_nodes(const char *path, bool isaddr, std::set<std::string> &addrs);
  long cut_idx(const char *pref, const std::string &cluster);

  int get_data(const char *path, std::string &data);
  int create_node(const char *path, int flags, const std::string &value, std::string &true_path);
  int delete_node(const char *path);
  int get_cluster_node(const char *path, bool ischeck, std::map< std::string, std::set<std::string> > &cfgs);
  int node_exist(const char *path);
  int check_add_create(const char *path, int flags, const std::string &value, std::string &true_path);

  int get_config(const char *path, std::map< std::string, std::set<std::string> > &cfgs);
  int get_error(const char *path, std::map< std::string, std::set<std::string> > &errs);
  int get_check(const char *path, std::map< std::string, std::map<std::string, std::string> > &chks);

  // check add logic
  int logic_check_add(const std::map< std::string, std::set<std::string> > &cfgs,
                      const std::map< std::string, std::set<std::string> > &errs,
                      const std::map< std::string, std::map<std::string, std::string> > &chks);

  int logic_error_rm(const std::map< std::string, std::set<std::string> > &cfgs,
                     const std::map< std::string, std::set<std::string> > &errs);

  int logic_check_rm(const std::map< std::string, std::set<std::string> > &cfgs,
                     const std::map< std::string, std::map<std::string, std::string> > &chks);


 public:

 RedisCtrl(LogOut *log,
	   const char *zk_addr,
	   const char *zk_root_path)
	 : log_(log), re_(log), zk_addr_(zk_addr),
    zk_root_path_(zk_root_path), zkh_(NULL),
    zk_lock_path_(zk_root_path_ + "/lock"),
    zk_cfg_path_(zk_root_path_ + "/config"),
    zk_err_path_(zk_root_path_ + "/error"),
    zk_check_path_(zk_root_path_ + "/check")

		{

		}
	LogOut *log() { return log_; }
  int start();

  void check();
};

#endif
