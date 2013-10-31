#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

#include "LogicCore.h"


using namespace std;

int LogicCore::syn_fn(int timeout, long uid, const proto_syn &proto, proto_idx_pair &idx)
{
  return syn_fn_ ? syn_fn_(timeout, uid, proto, idx) : 0;

}

int LogicCore::fin_fn(int timeout, long uid, const proto_fin &proto)
{
  return fin_fn_ ? fin_fn_(timeout, uid, proto) : 0;
}


int LogicCore::fin_delay_fn(int timeout, long uid, const proto_fin_delay &proto)
{
  return fin_delay_fn_ ? fin_delay_fn_(timeout, uid, proto) : 0;
}

int LogicCore::upidx_fn(int timeout, long uid, const proto_upidx &proto)
{
  return upidx_fn_ ? upidx_fn_(timeout, uid, proto) : 0;
}

int LogicCore::timeout_rm_fn(int timeout, int stamp, int count)
{
  return timeout_rm_fn_ ? timeout_rm_fn_(timeout, stamp, count) : 0;
}


void LogicCore::check_timeout()
{
  const char *fun = "LogicCore::check_timeout";
  for (;;) {
    int cn = timeout_rm_fn(300, time(NULL), -1);
    log_->info("%s-->rm count %d", fun, cn);
    sleep(1);
  }
}


void LogicCore::start()
{
  boost::thread td(boost::bind(&LogicCore::check_timeout, this));
  td.detach();
}

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


  // 以下各个协议解析代码需要结构优化
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
    pr.head.sublayer_index = sublayer_index;

    int rv = fin_fn(TIMEOUT_FIN, uid, pr);
    if (rv) {
      log_->error("%s-->uid:%ld rv:%d head:%d conn:%lu tp:%d",
                  fun, uid, rv, head_len, conn_idx, pro_tp);
    }

  } else if (PROTO_TYPE_FIN_DELAY == pro_tp) {
    if (pro.size() != PROTO_LEN_FIN_DELAY) {
      log_->info("%s->error fin_delay head:%d conn:%lu tp:%d", fun, head_len, conn_idx, pro_tp);
    }

    int sendidx = stream_ltt_bit32(&p, PROTO_LEN_SENDIDX);
    int recvidx = stream_ltt_bit32(&p, PROTO_LEN_RECVIDX);
    long uid = stream_ltt_bit64(&p, PROTO_LEN_UID);
    int expire = stream_ltt_bit32(&p, PROTO_LEN_STAMP);

    proto_fin_delay pr;
    pr.head.logic_conn = conn_idx;
    pr.head.idx.send_idx = sendidx;
    pr.head.idx.recv_idx = recvidx;
    pr.head.sublayer_index = sublayer_index;
    pr.expire = expire;

    int rv = fin_delay_fn(TIMEOUT_FIN_DELAY, uid, pr);
    if (rv) {
      log_->error("%s-->uid:%ld rv:%d head:%d conn:%lu tp:%d",
                  fun, uid, rv, head_len, conn_idx, pro_tp);
    }

  }

}

int LogicCore::expire_stamp(int st, int cli_tp)
{
  
  switch (cli_tp) {

  case 0:
  case 1:
    return st + 60 * 30;

  case 200:
  default:
    return st + 60 * 10;

  }

}


void LogicCore::from_sublayer_synok(long uid, long conn, int cli_tp, const string &ver, const string &sublayer_index, const map<string, string> &data)
{
  const char *fun = "LogicCore::from_sublayer_synok";
  proto_syn p;
  p.head.logic_conn = conn;
  p.head.idx.send_idx = 1;
  p.head.sublayer_index = sublayer_index;

  int ns = time(NULL);

  p.expire = expire_stamp(ns, cli_tp);

  p.cli_type = cli_tp;
  p.cli_ver = ver;

  vector<string> &kvs = p.data;

  for (map<string, string>::const_iterator it = data.begin();
       it != data.end(); ++it) {
    kvs.push_back(it->first);
    kvs.push_back(it->second);
  }


  proto_idx_pair ridx;
  int rv = syn_fn(TIMEOUT_SYN, uid, p, ridx);

  if (rv) {
    log_->error("%s-->%ld %d %s syn fail!", fun, uid, cli_tp, sublayer_index.c_str());
  }
}


