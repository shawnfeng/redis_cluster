#include <string>

#include <sys/syscall.h>
#include <arpa/inet.h>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>


#include "Util.h"

using namespace std;
uint64_t ipv4_int64(const char *ip, int port)
{
	uint64_t ipv4 = 0;
	uint64_t uip = inet_addr(ip);
	ipv4 = (uip << 32) | (uint32_t)port;

	return ipv4;
	
}

void int64_ipv4(uint64_t ipv4, char *ip, size_t len, int &port)
{
	uint64_t uip = ipv4 >> 32;
	port = ipv4 & 0x00000000FFFFFFFF;

	inet_ntop(AF_INET, &uip, ip, len);
	
}

bool str_ipv4(const std::string &addr, std::string &ip, int &port, std::string &err)
{
	size_t pos = addr.find(":");
	if (pos == std::string::npos) {
		err = "error addr can not find ':'";
		return false;
	}

	ip = addr.substr(0, pos);
	port = atoi(addr.substr(pos+1).c_str());

	if (port == 0 || ip.empty()) {
		err = "ip or port format error";
		return false;
	}

	return true;

}

bool str_ipv4_int64(const std::string &addr, uint64_t &ipv4, std::string &err)
{
  string ip;
  int port;
  if (!str_ipv4(addr, ip, port, err)) {
    return false;
  } else {
    ipv4 = ipv4_int64(ip.c_str(), port);
    return true;
  }
}

void int64_str_ipv4(uint64_t ipv4, std::string &addr)
{
  char ip[50];
  int port;
  int64_ipv4(ipv4, ip, sizeof(ip), port);

  char buff[100];
  snprintf(buff, sizeof(buff), "%s:%d", ip, port);
  addr = buff;

}


// ===================
void LogOut::log_trace(const char *log)
{
	
	printf("[T]:[%ld]:%s\n", syscall(SYS_gettid), log);
}


void LogOut::log_debug(const char *log)
{
	printf("[D]:[%ld]:%s\n", syscall(SYS_gettid), log);
}

void LogOut::log_info(const char *log)
{
	printf("[I]:[%ld]:%s\n", syscall(SYS_gettid), log);
}

void LogOut::log_warn(const char *log)
{
	printf("[W]:[%ld]:%s\n", syscall(SYS_gettid), log);
}

void LogOut::log_error(const char *log)
{
	printf("[E]:[%ld]:%s\n", syscall(SYS_gettid), log);
}

