#include <boost/lexical_cast.hpp>

#include "LogicCore.h"


using namespace std;

static const char *stream_anlys(const char *p, void *data, int len)
{
  // 暂时不考虑网络字节序问题
  memcpy(data, p, len);
  return p + len;
}

void LogicCore::from_sublayer(const string &sublayer_index, const string &pro)
{
  const char *fun = "LogicCore::from_sublayer";
  if (pro.size() < PROTO_LEN_GLOBAL_HEAD) {
    log_->error("%s->error len index:%s", fun, sublayer_index.c_str());
    return;
  }



  int head_len = 0;
  long conn_idx = 0;
  int pro_tp = 0;

  const char *p = pro.c_str();

  p = stream_anlys(p, &head_len, PROTO_LEN_HEAD);
  p = stream_anlys(p, &conn_idx, PROTO_LEN_CONN);
  p = stream_anlys(p, &pro_tp, PROTO_LEN_TYPE);

  log_->info("%s->head:%d conn:%lu tp:%d", fun, head_len, conn_idx, pro_tp);
  if (PROTO_TYPE_FIN == pro_tp) {
    if (pro.size() != PROTO_LEN_FIN) {
      log_->info("%s->error fin head:%d conn:%lu tp:%d", fun, head_len, conn_idx, pro_tp);
    }

    int sendidx = 0;
    int recvidx = 0;
    long uid = 0;
    p = stream_anlys(p, &sendidx, PROTO_LEN_SENDIDX);
    p = stream_anlys(p, &recvidx, PROTO_LEN_RECVIDX);
    p = stream_anlys(p, &uid, PROTO_LEN_UID);

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


