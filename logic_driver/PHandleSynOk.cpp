#include <vector>

#include "PHandleBase.h"

using namespace std;
using namespace google::protobuf;

void PHandleSynOk::process(LogicCore *lc, const std::string &sublayer_index, const ProHead &ph)
{
  const char *fun = "PHandleSynOk::process";
  //lc->log()->info("%s-->%s %s", fun, sublayer_index.c_str(), ph.DebugString().c_str());

  ProSynOk ps;
  if (!ps.ParseFromString(ph.sub_pro())) {
    lc->log()->error("%s-->error parser %s", fun, sublayer_index.c_str());
  }

  //lc->log()->info("%s-->%s %s", fun, sublayer_index.c_str(), ps.DebugString().c_str());
  long uid = ph.uid();
  if (uid <= 0) {
    lc->log()->warn("%s-->err uid:%ld", fun, uid);
    return;
  }

  lc->log()->info("%s-->U:%ld C:%ld G:%s", fun, uid, ph.conn(), sublayer_index.c_str());

  proto_syn p;
  p.head.logic_conn = ph.conn();
  p.head.idx.send_idx = ph.sid();
  p.head.sublayer_index = sublayer_index;

  int ns = lc->time_now_fn();
  p.expire = ns;

  p.cli_type = ps.cli_tp();
  p.cli_ver = ps.cli_ver();

  vector<string> &kvs = p.data;

  const RepeatedPtrField<ExtCliData> &ecd = ps.ext();
  for (RepeatedPtrField<ExtCliData>::const_iterator it = ecd.begin();
       it != ecd.end(); ++it) {
    kvs.push_back(it->key());
    kvs.push_back(it->value());
  }


  proto_idx_pair ridx;
  int rv = lc->syn_fn(LogicCore::TIMEOUT_SYN, uid, p, ridx);

  if (rv) {
    lc->log()->error("%s-->%ld %d %s syn fail!", fun, uid, ps.cli_tp(), sublayer_index.c_str());
  }


}


