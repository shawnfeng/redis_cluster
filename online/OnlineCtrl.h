#ifndef __ONLINE_CTRL_H_H__
#define __ONLINE_CTRL_H_H__

#include <string>

#include "../src/RedisClient.h"

class OnlineCtrl {
  struct script_t {
    std::string sha1;
    std::string data;
  };

	LogOut log_;
  std::string sp_;
	RedisClient rc_;

  script_t s_online_;
  script_t s_offline_;
  script_t s_session_info_;
  script_t s_sessions_;


 public:
 OnlineCtrl(void (*log_t)(const char *),
            void (*log_d)(const char *),
            void (*log_i)(const char *),
            void (*log_w)(const char *),
            void (*log_e)(const char *),

            const char *script_path
            );

	void online(long uid, const std::string &session, const std::vector<std::string> &kvs);
	void offline(long uid, const std::string &session);
	void get_sessions(long uid, std::vector<std::string> &sessions);
	void get_session_info(long uid, const std::string &session, const std::vector<std::string> &ks,
                        std::map<std::string, std::string> &kvs);

};


#endif
