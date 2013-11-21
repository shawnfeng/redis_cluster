#ifndef __REDIS_EVENT_H_H__
#define __REDIS_EVENT_H_H__
#include <vector>
#include <set>
#include <map>

#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "libev.h"

#include "Util.h"

class RedisEvent;

class cflag_t {
	int f_;
	void *d_;
public:
	cflag_t() : f_(0), d_(NULL) {}
	void reset() { f_ = 0; d_ = NULL; }

	int f() { return f_; }
	int d_f() { return --f_; }
	void *d() { return d_; }

	void set_f(int f) { f_ = f; }
	void set_d(void *d) { d_ = d; }
};


class userdata_t {

	enum { CUR_CALL_NUM = 1000000, };
	size_t idx_;
	cflag_t cfg_[CUR_CALL_NUM];
	std::map<uint64_t, redisAsyncContext *> ctxs_;
	RedisEvent *re_;

 private:


public:
 userdata_t(RedisEvent *re) : re_(re)
		{
		}

	RedisEvent *re() { return re_; }

  inline cflag_t *get_cf();

	void insert(uint64_t addr, redisAsyncContext *c);
	void clear(uint64_t addr);
	redisAsyncContext *lookup(uint64_t addr);


};

struct RedisRv {
  int type; /* REDIS_REPLY_* */
  long long integer; /* The integer when type is REDIS_REPLY_INTEGER */
  std::string str; /* Used for both REDIS_REPLY_ERROR and REDIS_REPLY_STRING */
  std::vector<RedisRv> element;

  void dump(LogOut *logout, const char *pref, int lv, int depth = 0) const;
};

typedef std::map<uint64_t, RedisRv> RedisRvs;

class AsynReqCheck {
  std::multimap<double, cflag_t *> req_;
 public:
  void insert(double stamp, cflag_t *cf) {
    //printf("AsynReqCheck::insert %lf %p\n", stamp, cf);
    req_.insert(std::pair<double, cflag_t *>(stamp, cf)); 
  }
  void erase(double stamp, cflag_t *cf)
  {
    //printf("AsynReqCheck::erase %lf %p\n", stamp, cf);
    std::pair <std::multimap<double, cflag_t *>::iterator, std::multimap<double, cflag_t *>::iterator> ret;
    ret = req_.equal_range(stamp);

    for (std::multimap<double, cflag_t *>::iterator it=ret.first; it!=ret.second;) {
      if (it->second == cf) {
        req_.erase(it++);
        break;
      } else {
        it++;
      }
    }
  }

  void timeout_callback(double stamp);
};

class RedisEvent {
 private:
  enum {
    ARGV_MAX_LEN = 2048,
  };
	LogOut *log_;
	struct ev_loop *loop_;

	ev_async async_w_;
  ev_timer periodic_check_w_;

	boost::mutex mutex_;
	userdata_t ud_;


  AsynReqCheck req_check_;
  // argv
  const char **cmd_argv_;
  size_t *cmd_argvlen_;

 private:
	void run();
	void start ();
 public:
	RedisEvent(LogOut *log
		   ) : log_(log), loop_(EV_DEFAULT), ud_(this)
    , cmd_argv_(NULL), cmd_argvlen_(NULL) 
		{
      cmd_argv_ = new const char *[ARGV_MAX_LEN];
      cmd_argvlen_ = new size_t[ARGV_MAX_LEN];
    
			log->info("%s-->loop:%p", "RedisEvent::RedisEvent", loop_);
      start();
      
		}
  ~RedisEvent()
    {
      if (cmd_argv_) delete [] cmd_argv_;
      if (cmd_argvlen_) delete cmd_argvlen_;
    }

	void lock() { mutex_.lock(); }
	void unlock() { mutex_.unlock(); }


	void connect(uint64_t addr);
  void async_timeout(double stamp) { req_check_.timeout_callback(stamp); };
  void async_erase(double stamp, cflag_t *cf) { req_check_.erase(stamp, cf); };


	LogOut *log() { return log_; }

	void cmd(RedisRvs &rv,
           const char *log_key,
           const std::map< uint64_t, std::vector<std::string> > &addr_cmd,
           int timeout,
           const std::string &lua_code, bool iseval
           );

	void cmd_async(long key, void *data, void (*callback)(const RedisRvs &, double, bool, long, void *),
           const char *log_key,
           const std::map< uint64_t, std::vector<std::string> > &addr_cmd,
           double timeout
                 //           const std::string &lua_code, bool iseval
           );
  void *add_repeat_timer(void *data, void (*cb)(void *), double intv);
  void rm_repeat_timer(void *timer);

	struct ev_loop *loop() { return loop_; }
  double time_now() { return ev_now(EV_DEFAULT); }

};



#endif
