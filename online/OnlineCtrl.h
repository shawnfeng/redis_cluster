#ifndef __ONLINE_CTRL_H_H__
#define __ONLINE_CTRL_H_H__

#include <string>

#include "../src/RedisEvent.h"
#include "../src/RedisHash.h"

#include "../logic_driver/HookDef.h"

class script_t {
  std::string sha1_;
  std::string data_;

  bool need_load_;
	boost::shared_mutex mux_;
  std::set<uint64_t> addrs_;

public:
 script_t() : need_load_(false) {}

  void set_scp(const char *s, const char *d)
  {
    sha1_ = s;
    data_ = d;
  }

  void add(uint64_t addr)
  {
    boost::unique_lock< boost::shared_mutex > lock(mux_);
    addrs_.insert(addr);
    need_load_ = true;
  }

  const std::string &sha1() const { return sha1_; }
  const std::string &data() const { return data_; }

  const std::string &lookup_sha1(RedisEvent *re);

};


class OnlineCtrl {

	LogOut *log_;
	RedisEvent *re_;
	RedisHash *rh_;


  std::string sp_;

  script_t s_session_info_;
  script_t s_sessions_;
  script_t s_multi_;
  script_t s_timeout_rm_;

  script_t s_syn_;
  script_t s_fin_;
  script_t s_fin_delay_;
  script_t s_upidx_;



 private:
  void single_uid_commend(const char *fun,
                          int timeout,
                          const std::string &suid, std::vector<std::string> &args,
                          const std::string &lua_code,
                          RedisRvs &rv);

  void single_uid_commend_async(const char *fun,
                                long uid,
                                int type,
                                double timeout,
                                std::vector<std::string> &args
                                );

  void one_uid_session(long actor, const RedisRv &rv, std::map< long, std::map< std::string, std::map<std::string, std::string> > > &uids_sessions);
  void load_script(const std::string &path, script_t &scp);
  int gen_proto_args(int tp, const void *proto, std::vector<std::string> &args);
  void gen_redis_args(int tp, long suid, const void *proto, std::vector<std::string> &args);

 public:
 OnlineCtrl(
            LogOut *log,

            RedisEvent *re,
            RedisHash *rh,

            const char *script_path
            );

 //-------------------logic connection level interface-----------------------
 int syn(int timeout, long uid, const proto_syn &proto, proto_idx_pair &idx);
 int fin(int timeout, long uid, const proto_fin &proto, std::string &cli_info);
 int fin_delay(int timeout, long uid, const proto_fin_delay &proto);
 int upidx(int timeout, long uid, const proto_upidx &proto, proto_idx_pair &idx);
 int timeout_rm(int timeout, int stamp, int count, std::vector< std::pair<long, std::string> > &rvs);
 // async interface
 int syn_async(double timeout, long uid, const proto_syn &proto);
 int fin_async(double timeout, long uid, const proto_fin &proto);
 int fin_delay_async(double timeout, long uid, const proto_fin_delay &proto);
 int upidx_async(double timeout, long uid, const proto_upidx &proto);

 // void msg(int timeout, long uid, const proto_msg &proto);
 //--------bussiness level interface--------------------
 // ...

 //-----------------------------
 /*
 void online(int timeout, long uid, const std::string &session, int type, const std::vector<std::string> &kvs);
	void offline(int timeout, long uid, const std::string &session);
 */
	void get_sessions(int timeout, long uid, std::vector<std::string> &sessions);
	void get_session_info(int timeout, long uid, const std::string &session, const std::vector<std::string> &ks,
                        std::map<std::string, std::string> &kvs);

	void get_multi(int timeout, long actor, const std::vector<long> &uids,
                 std::map< long, std::map< std::string, std::map<std::string, std::string> > > &uids_sessions
                 );


  LogOut *log() { return log_; }
  int rv_check_syn(long uid, const RedisRvs &rv, proto_idx_pair &idx);
  int rv_check_fin(long uid, const RedisRvs &rv, std::string &cli_info);
  int rv_check_fin_delay(long uid, const RedisRvs &rv);
  int rv_check_upidx(long uid, const RedisRvs &rv, proto_idx_pair &idx);

  void check_scritp_load(int type, const RedisRvs &rv);
  void need_load_script(int type, uint64_t addr);

};


#endif
