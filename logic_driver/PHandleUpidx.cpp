#include "PHandleBase.h"

void PHandleUpidx::process(LogicCore *lc, const std::string &sublayer_index, const ProHead &ph)
{
  const char *fun = "PHandleUpidx::process";
  //lc->log()->info("%s-->%s %s", fun, sublayer_index.c_str(), ph.DebugString().c_str());

  ProUpidx ps;
  if (!ps.ParseFromString(ph.sub_pro())) {
    lc->log()->error("%s-->error parser %s", fun, sublayer_index.c_str());
  }

  //lc->log()->info("%s-->%s %s", fun, sublayer_index.c_str(), ps.DebugString().c_str());

  long conn_idx = ph.conn();
  int sendidx = ph.sid();
  int recvidx = ph.rid();
  long uid = ph.uid();
  int expire = lc->time_now_fn();
  int cli_tp = ps.cli_tp();
    //log_->info("########### %d %d %ld", cli_tp, expire, uid);
  if (uid <= 0) {
    lc->log()->warn("%s-->err uid:%ld", fun, uid);
    return;
  }

  lc->log()->info("%s-->U:%ld C:%ld G:%s", fun, uid, ph.conn(), sublayer_index.c_str());

  proto_upidx pr;
  pr.head.logic_conn = conn_idx;
  pr.head.idx.send_idx = sendidx;
  pr.head.idx.recv_idx = recvidx;
  pr.head.sublayer_index = sublayer_index;
  pr.expire = expire;
  pr.cli_type = cli_tp;

  proto_idx_pair ridx;
  int rv = lc->upidx_fn(LogicCore::TIMEOUT_UPIDX, uid, pr, ridx);
    //    log_->info("##########-->rv:%d, sid:%d rid:%d", rv, ridx.send_idx, ridx.recv_idx);
  if (rv) {
    lc->log()->error("%s-->uid:%ld rv:%d conn:%ld",
                fun, uid, rv, conn_idx);
  }

}


