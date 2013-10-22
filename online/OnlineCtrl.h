#ifndef __ONLINE_CTRL_H_H__
#define __ONLINE_CTRL_H_H__

#include <string>

#include "../src/RedisEvent.h"
#include "../src/RedisHash.h"

class OnlineCtrl {
  struct script_t {
    std::string sha1;
    std::string data;
  };

	LogOut log_;
	RedisEvent re_;
	RedisHash rh_;


  std::string sp_;

  script_t s_online_;
  script_t s_offline_;
  script_t s_session_info_;
  script_t s_sessions_;
  script_t s_multi_;
  script_t s_timeout_rm_;

 private:
  void single_uid_commend(const char *fun,
                          int timeout,
                          const std::string &suid, std::vector<std::string> &args,
                          const std::string &lua_code,
                          RedisRvs &rv);

  void one_uid_session(long actor, const RedisRv &rv, std::map< long, std::map< std::string, std::map<std::string, std::string> > > &uids_sessions);
 public:
 OnlineCtrl(void (*log_t)(const char *),
            void (*log_d)(const char *),
            void (*log_i)(const char *),
            void (*log_w)(const char *),
            void (*log_e)(const char *),

            const char *zk_addr,
            const char *zk_path,

            const char *script_path
            );

 void online(int timeout, long uid, const std::string &session, const std::vector<std::string> &kvs);
	void offline(int timeout, long uid, const std::string &session);
	void get_sessions(int timeout, long uid, std::vector<std::string> &sessions);
	void get_session_info(int timeout, long uid, const std::string &session, const std::vector<std::string> &ks,
                        std::map<std::string, std::string> &kvs);

	void get_multi(int timeout, long actor, const std::vector<long> &uids,
                 std::map< long, std::map< std::string, std::map<std::string, std::string> > > &uids_sessions
                 );

  int timeout_rm(int timeout, int stamp, int idx, int count);

};


#endif
