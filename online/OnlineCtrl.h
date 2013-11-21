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
enum {
  REQ_SYN,
  REQ_FIN,
  REQ_FIN_DELAY,
  REQ_UPIDX,

  REQ_COUNT,
};



typedef void (*stat_cb_t)(const char *key, double accum, int calls, int tmouts, double max);
class CallTimeStat {
  struct stat_t {
    double accum;
    int calls;
    int timeout;
    double max;
  stat_t() : accum(0.0), calls(0), timeout(0), max(0.0) {}
    void clear()
    {
      accum = 0.0;
      calls = 0;
      timeout = 0;
      max = 0.0;
    }
  };
  std::map<std::string, stat_t> sts_;

  stat_t v_sts_[REQ_COUNT];
  const char *v_sts_key_[REQ_COUNT];
 public:
  CallTimeStat()
    {
      for (int i = 0; i < REQ_COUNT; ++i) {
        if (REQ_SYN == i) {
          v_sts_key_[i] = "redis.syn";
        } else if (REQ_FIN == i) {
          v_sts_key_[i] = "redis.fin";
        } else if (REQ_FIN_DELAY == i) {
          v_sts_key_[i] = "redis.fin_delay";
        } else if (REQ_UPIDX == i) {
          v_sts_key_[i] = "redis.upidx";
        }
      }
    }
  void stat2(int type, double tm, bool istmout)
  {
    if (type > REQ_COUNT) return;
    stat_t &st = v_sts_[type];

    ++st.calls;
    st.accum += tm;
    if (istmout) ++st.timeout;
    if(st.max < tm) st.max = tm;

  }

  // same thread to run
  void stat(const char *key, double tm, bool istmout)
  {
    std::map<std::string, stat_t>::iterator it = sts_.find(key);
    if (it == sts_.end()) {
      sts_[key] = stat_t();
      it = sts_.begin();
    }

    stat_t &st = it->second;

    ++st.calls;
    st.accum += tm;
    if (istmout) ++st.timeout;
    if(st.max < tm) st.max = tm;

  }

  void show2(stat_cb_t cb)
  {
    for (int i = 0; i < REQ_COUNT; ++i) {
      stat_t &st = v_sts_[i];
      if (st.calls > 0) {
        cb(v_sts_key_[i], st.accum, st.calls, st.timeout, st.max);
        st.clear();
      }

    }
  }


  void show(stat_cb_t cb)
  {
    for (std::map<std::string, stat_t>::iterator it = sts_.begin();
         it != sts_.end(); ++it) {
      stat_t &st = it->second;
      if (st.calls > 0) {
        cb(it->first.c_str(), st.accum, st.calls, st.timeout, st.max);
        st.clear();
      }
    }
  }

  

};

class OnlineCtrl {

	LogOut *log_;
	RedisEvent *re_;
	RedisHash *rh_;

  void *stat_timer_;
  stat_cb_t stat_cb_;


  CallTimeStat call_stat_;

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

            stat_cb_t stat_cb,
            const char *script_path

            );

 ~OnlineCtrl()
   {
     if (stat_timer_) re_->rm_repeat_timer(stat_timer_);
   }


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

  void stat_cb_add(int type, double tm, bool istmout)
  {
    call_stat_.stat2(type, tm, istmout);
  }
  void stat_cb_out()
  {
    call_stat_.show2(stat_cb_);
  }

};


#endif
