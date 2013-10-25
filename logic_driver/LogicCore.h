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
  hook_upidx_fn upidx_fn_;

 public:
 LogicCore(LogOut *log,

           hook_syn_fn syn,
           hook_fin_fn fin,
           hook_upidx_fn upidx
           ) :
  log_(log),
    syn_fn_(syn), fin_fn_(fin), upidx_fn_(upidx)
  {

  }

  enum {
    TIMEOUT_SYN = 100,
    TIMEOUT_FIN = 100,
  };

  enum {
    PROTO_TYPE_SYN = 1,
    PROTO_TYPE_FIN = 2,
  };

  enum {
    PROTO_LEN_HEAD = 2,
    PROTO_LEN_CONN = 8,
    PROTO_LEN_TYPE = 2,

    PROTO_LEN_SENDIDX = 4,
    PROTO_LEN_RECVIDX = 4,
    PROTO_LEN_UID = 8,

    PROTO_LEN_GLOBAL_HEAD = PROTO_LEN_HEAD + PROTO_LEN_CONN + PROTO_LEN_TYPE,
    PROTO_LEN_FIN = PROTO_LEN_GLOBAL_HEAD + PROTO_LEN_SENDIDX + PROTO_LEN_RECVIDX + PROTO_LEN_UID,
  };


  // interface
  void from_sublayer(const std::string &sublayer_index, const std::string &pro);
  void from_sublayer_synok(long uid, int cli_tp, long conn, const std::string &sublayer_index, const std::map<std::string, std::string> &data);

  void from_toplayer();


};




#endif
