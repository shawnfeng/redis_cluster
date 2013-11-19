#include "PHandleBase.h"

void PHandleFinDelay::process(LogicCore *lc, const std::string &sublayer_index, const ProHead &ph)
{
  const char *fun = "PHandleFinDelay::process";
  //lc->log()->info("%s-->%s %s", fun, sublayer_index.c_str(), ph.DebugString().c_str());

  ProFinDelay ps;
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


  int expire = ps.expire();

  proto_fin_delay pr;
  pr.head.logic_conn = conn_idx;
  pr.head.idx.send_idx = sendidx;
  pr.head.idx.recv_idx = recvidx;
  pr.head.sublayer_index = sublayer_index;
  pr.expire = expire;

  int rv = lc->fin_delay_fn(LogicCore::TIMEOUT_FIN_DELAY, uid, pr);
  if (rv) {
    lc->log()->error("%s-->uid:%ld rv:%d conn:%ld",
                  fun, uid, rv, conn_idx);
  }


}


