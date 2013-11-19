#include <string>

#include "PHandleBase.h"

using namespace std;

void PHandleFin::process(LogicCore *lc, const std::string &sublayer_index, const ProHead &ph)
{
  const char *fun = "PHandleFin::process";
  //lc->log()->info("%s-->%s %s", fun, sublayer_index.c_str(), ph.DebugString().c_str());

  ProFin ps;
  if (!ps.ParseFromString(ph.sub_pro())) {
    lc->log()->error("%s-->error parser %s", fun, sublayer_index.c_str());
  }

  //lc->log()->info("%s-->%s %s", fun, sublayer_index.c_str(), ps.DebugString().c_str());


  long conn_idx = ph.conn();
  int sendidx = ph.sid();
  int recvidx = ph.rid();
  long uid = ph.uid();
  if (uid <= 0) {
    lc->log()->warn("%s-->err uid:%ld", fun, uid);
    return;
  }


  proto_fin pr;
  pr.head.logic_conn = conn_idx;
  pr.head.idx.send_idx = sendidx;
  pr.head.idx.recv_idx = recvidx;
  pr.head.sublayer_index = sublayer_index;

  string cli_info;
  int rv = lc->fin_fn(LogicCore::TIMEOUT_FIN, uid, pr, cli_info);
  if (rv) {
      // 
    if (1 == rv) {
      lc->log()->info("%s-->nil online uid:%ld rv:%d conn:%ld",
                 fun, uid, rv, conn_idx);
    } else {
      lc->log()->warn("%s-->invalid uid:%ld rv:%d conn:%ld",
                 fun, uid, rv, conn_idx);
    }
  } else {
    rv = lc->offline_notify_fn(uid, cli_info);
    if (rv) {
      lc->log()->error("%s-->offline_notify_fn uid:%ld rv:%d conn:%ld",
                  fun, uid, rv, conn_idx);
    }
  }


}


