#ifndef __REDIS_UTIL_H_H__
#define __REDIS_UTIL_H_H__
#include <stdarg.h>
#include <stdio.h>

#include <stdint.h>


class LogOut {
	void (*log_t_)(const char *);
	void (*log_d_)(const char *);
	void (*log_i_)(const char *);
	void (*log_w_)(const char *);
	void (*log_e_)(const char *);

	enum { BUFFER_SIZE = 1024, };

#define LOGOUT_FMT_LOG(format, log_f)				\
	if (log_f) {						\
		char buff[BUFFER_SIZE];				\
		va_list args;					\
		va_start(args, format);				\
		vsnprintf(buff, BUFFER_SIZE, format, args);	\
		va_end(args);					\
		log_f(buff);					\
	}

 public:
	LogOut(
	       void (*log_t)(const char *),
	       void (*log_d)(const char *),
	       void (*log_i)(const char *),
	       void (*log_w)(const char *),
	       void (*log_e)(const char *)
	       ) : log_t_(log_t), log_d_(log_d), log_i_(log_i), log_w_(log_w), log_e_(log_e)
	{}

	void trace(const char *format, ...) { LOGOUT_FMT_LOG(format, log_t_) }
	void debug(const char *format, ...) { LOGOUT_FMT_LOG(format, log_d_) }
	void info(const char *format, ...) { LOGOUT_FMT_LOG(format, log_i_) }
	void warn(const char *format, ...) { LOGOUT_FMT_LOG(format, log_w_) }
	void error(const char *format, ...) { LOGOUT_FMT_LOG(format, log_e_) }


	// just simple output stdout
	static void log_trace(const char *log);
	static void log_debug(const char *log);
	static void log_info(const char *log);
	static void log_warn(const char *log);
	static void log_error(const char *log);


 LogOut() : log_t_(LogOut::log_trace),
		log_d_(LogOut::log_debug),
		log_i_(LogOut::log_info),
		log_w_(LogOut::log_warn),
		log_e_(LogOut::log_error)
	{}

};

uint64_t ipv4_int64(const char *ip, int port);

void int64_ipv4(uint64_t ipv4, char *ip, size_t len, int &port);

bool str_ipv4(const std::string &addr, std::string &ip, int &port, std::string &err);

bool str_ipv4_int64(const std::string &addr, uint64_t &ipv4, std::string &err);
void int64_str_ipv4(uint64_t ipv4, std::string &addr);

class TimeUse {
  long bt_;

 public:

  long get_mtime() const
  {
    timeval tstamp;
    gettimeofday(&tstamp, NULL);
    return (tstamp.tv_sec*1000000 + tstamp.tv_usec)/1000;
  }

  
 TimeUse() : bt_(get_mtime()) {}

  long intv() const { 
    return get_mtime() - bt_;
  }
  long intv_reset() {
    long nw = get_mtime();
    long ut = nw - bt_;
    bt_ = nw;
    return ut;
  }
  
};



#endif
