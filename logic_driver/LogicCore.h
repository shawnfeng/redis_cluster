#ifndef __LOGIC_CORE_H_H__
#define __LOGIC_CORE_H_H__

#include <string>
#include <vector>
#include <map>

#include "../src/Util.h"
#include "HookDef.h"

class LogicCore {
	LogOut *log_;

  hook_syn_fn syn_fn_;
  hook_fin_fn fin_fn_;
  hook_fin_delay_fn fin_delay_fn_;
  hook_upidx_fn upidx_fn_;
  hook_timeout_rm_fn timeout_rm_fn_;
  hook_offline_notify_fn offline_notify_fn_;
  hook_offline_notify_multi_fn offline_notify_multi_fn_;


 private:
  void check_timeout();
  int expire_stamp(int st, int cli_tp);

  int syn_fn(int timeout, long uid, const proto_syn &proto, proto_idx_pair &idx);
  int fin_fn(int timeout, long uid, const proto_fin &proto, std::string &cli_info);
  int fin_delay_fn(int timeout, long uid, const proto_fin_delay &proto);
  int upidx_fn(int timeout, long uid, const proto_upidx &proto, proto_idx_pair &idx);
  int timeout_rm_fn(int timeout, int stamp, int count, std::vector< std::pair<long, std::string> > &rvs);
  int offline_notify_fn(long uid, std::string &cli_info);
  int offline_notify_multi_fn(std::vector< std::pair<long, std::string> > &rvs);


  void start();
 public:
 LogicCore(LogOut *log,

           hook_syn_fn syn,
           hook_fin_fn fin,
           hook_fin_delay_fn fin_delay,
           hook_upidx_fn upidx,
           hook_timeout_rm_fn timeout_rm,
           hook_offline_notify_fn offline_notify,
           hook_offline_notify_multi_fn offline_notify_multi

           ) :
  log_(log),
    syn_fn_(syn),
    fin_fn_(fin),
    fin_delay_fn_(fin_delay),
    upidx_fn_(upidx),
    timeout_rm_fn_(timeout_rm),
    offline_notify_fn_(offline_notify),
    offline_notify_multi_fn_(offline_notify_multi)

  {
    start();
  }

  enum {
    TIMEOUT_SYN = 100,
    TIMEOUT_FIN = 100,
    TIMEOUT_FIN_DELAY = 100,
    TIMEOUT_UPIDX = 100,
  };

  enum {
    PROTO_TYPE_SYN = 1,
    PROTO_TYPE_FIN = 2,
    PROTO_TYPE_FIN_DELAY = 3,
    PROTO_TYPE_UPIDX = 4,
  };

  enum {
    PROTO_LEN_HEAD = 2,
    PROTO_LEN_CONN = 8,
    PROTO_LEN_TYPE = 2,

    PROTO_LEN_SENDIDX = 4,
    PROTO_LEN_RECVIDX = 4,
    PROTO_LEN_UID = 8,
    PROTO_LEN_STAMP = 4,


    PROTO_LEN_GLOBAL_HEAD = PROTO_LEN_HEAD + PROTO_LEN_CONN + PROTO_LEN_TYPE,

    PROTO_LEN_FIN = PROTO_LEN_GLOBAL_HEAD + PROTO_LEN_SENDIDX + PROTO_LEN_RECVIDX + PROTO_LEN_UID,
    PROTO_LEN_FIN_DELAY = PROTO_LEN_GLOBAL_HEAD + PROTO_LEN_SENDIDX + PROTO_LEN_RECVIDX + PROTO_LEN_UID + PROTO_LEN_STAMP,
    PROTO_LEN_UPIDX = PROTO_LEN_GLOBAL_HEAD + PROTO_LEN_SENDIDX + PROTO_LEN_RECVIDX + PROTO_LEN_UID + PROTO_LEN_STAMP,
  };




  // interface
  void from_sublayer(const std::string &sublayer_index, const std::string &pro);
  void from_sublayer_synok(long uid, long conn, int cli_tp, const std::string &ver, const std::string &sublayer_index, const std::map<std::string, std::string> &data);




};




#endif
