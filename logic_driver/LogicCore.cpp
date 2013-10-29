#include <boost/lexical_cast.hpp>

#include "LogicCore.h"


using namespace std;


void LogicCore::from_sublayer(const string &sublayer_index, const string &pro)
{
  const char *fun = "LogicCore::from_sublayer";
  if (pro.size() < PROTO_LEN_GLOBAL_HEAD) {
    log_->error("%s->error len index:%s", fun, sublayer_index.c_str());
    return;
  }


  const char *p = pro.c_str();

  int head_len = stream_ltt_bit32(&p, PROTO_LEN_HEAD);
  long conn_idx = stream_ltt_bit64(&p, PROTO_LEN_CONN);
  int pro_tp = stream_ltt_bit32(&p, PROTO_LEN_TYPE);

  log_->info("%s->head:%d conn:%lu tp:%d", fun, head_len, conn_idx, pro_tp);
  if (PROTO_TYPE_FIN == pro_tp) {
    if (pro.size() != PROTO_LEN_FIN) {
      log_->info("%s->error fin head:%d conn:%lu tp:%d", fun, head_len, conn_idx, pro_tp);
    }

    int sendidx = stream_ltt_bit32(&p, PROTO_LEN_SENDIDX);
    int recvidx = stream_ltt_bit32(&p, PROTO_LEN_RECVIDX);
    long uid = stream_ltt_bit64(&p, PROTO_LEN_UID);

    proto_fin pr;
    pr.head.logic_conn = conn_idx;
    pr.head.idx.send_idx = sendidx;
    pr.head.idx.recv_idx = recvidx;

    int rv = fin_fn_(TIMEOUT_FIN, uid, pr);
    if (rv) {
      log_->error("%s-->uid:%ld rv:%d head:%d conn:%lu tp:%d",
                  fun, uid, rv, head_len, conn_idx, pro_tp);
    }

  }

}

void LogicCore::from_sublayer_synok(long uid, int cli_tp, long conn, const string &sublayer_index, const map<string, string> &data)
{
  const char *fun = "LogicCore::from_sublayer_synok";
  proto_syn p;
  p.head.logic_conn = conn;
  p.head.idx.send_idx = 1;

  p.expire = time(NULL) + 100; // 过期时间先被写死了！！！
  p.sublayer_index = sublayer_index;
  p.cli_type = cli_tp;

  vector<string> &kvs = p.data;

  for (map<string, string>::const_iterator it = data.begin();
       it != data.end(); ++it) {
    kvs.push_back(it->first);
    kvs.push_back(it->second);
  }


  proto_idx_pair ridx;
  int rv = syn_fn_(TIMEOUT_SYN, uid, p, ridx);

  if (rv) {
    log_->error("%s-->%ld %d %s syn fail!", fun, uid, cli_tp, sublayer_index.c_str());
  }
}


