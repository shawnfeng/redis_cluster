#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

#include "LogicConnLayerProto.pb.h"

#include "PHandleBase.h"

#include "LogicCore.h"



using namespace std;

int LogicCore::syn_fn(int timeout, long uid, const proto_syn &proto, proto_idx_pair &idx)
{
  return syn_fn_ ? syn_fn_(timeout, uid, proto, idx) : 0;

}

int LogicCore::fin_fn(int timeout, long uid, const proto_fin &proto, std::string &cli_info)
{
  return fin_fn_ ? fin_fn_(timeout, uid, proto, cli_info) : 0;
}


int LogicCore::fin_delay_fn(int timeout, long uid, const proto_fin_delay &proto)
{
  return fin_delay_fn_ ? fin_delay_fn_(timeout, uid, proto) : 0;
}

int LogicCore::upidx_fn(int timeout, long uid, const proto_upidx &proto, proto_idx_pair &idx)
{
  return upidx_fn_ ? upidx_fn_(timeout, uid, proto, idx) : 0;
}

int LogicCore::timeout_rm_fn(int timeout, int stamp, int count, std::vector< std::pair<long, std::string> > &rvs)
{
  return timeout_rm_fn_ ? timeout_rm_fn_(timeout, stamp, count, rvs) : 0;
}

int LogicCore::offline_notify_fn(long uid, std::string &cli_info)
{
  return offline_notify_fn_ ? offline_notify_fn_(uid, cli_info) : 0;
}

int LogicCore::offline_notify_multi_fn(std::vector< std::pair<long, std::string> > &rvs)
{
  return offline_notify_multi_fn_ ? offline_notify_multi_fn_(rvs) : 0;
}

void LogicCore::callstat_fn(const char *stat_key, int tm, int rev)
{
  if (callstat_fn_) {
    callstat_fn_(stat_key, tm, rev);
  }

}
int LogicCore::time_now_fn()
{
  return time_now_fn_ ? (int)time_now_fn_() : time(NULL);
}


void LogicCore::check_timeout()
{
  const char *fun = "LogicCore::check_timeout";
  vector< pair<long, string> > rvs;
  int stat_cb_stamp = time_now_fn();
  for (;;) {
    TimeUse tu;
    const char *stat_key = "timeout_rm";
    rvs.clear();
    int rv = timeout_rm_fn(300, time_now_fn(), -1, rvs);
    //    log_->debug("%s-->rv:%d sz:%lu", rv, rvs.size());
    if (rv) {
      log_->error("%s-->timeout rm rv %d", fun, rv);
    } else {
      rv = offline_notify_multi_fn(rvs);
      if (rv) log_->error("%s-->offline_notify_multi_fn rv %d", fun, rv);
    }

    log_->info("%s-->rm count %lu", fun, rvs.size());
    callstat_fn(stat_key, tu.intv(), 0);

    int now = time_now_fn();
    if (now-stat_cb_stamp >= 10) {
      oc_->stat_cb_out();
      stat_cb_stamp = now;
    }

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
  TimeUse tu;
  const char *stat_key = "unknown_proto";
  ProHead pb;
  if (!pb.ParseFromString(pro)) {
    log_->error("%s-->error parser %s", fun, sublayer_index.c_str());
    return;
  }

  int type = pb.type();

  boost::shared_ptr<PHandleBase> ph;
  switch (type) {
  case ProHead::TYPE_SYN:
    ph = boost::shared_ptr<PHandleBase>(new PHandleSyn);
    stat_key = "syn";
    break;

  case ProHead::TYPE_SYNOK:
    ph = boost::shared_ptr<PHandleBase>(new PHandleSynOk);
    stat_key = "synok";
    break;

  case ProHead::TYPE_UPIDX:
    ph = boost::shared_ptr<PHandleBase>(new PHandleUpidx);
    stat_key = "upidx";
    break;

  case ProHead::TYPE_FIN:
    ph = boost::shared_ptr<PHandleBase>(new PHandleFin);
    stat_key = "fin";
    break;

  case ProHead::TYPE_FIN_DELAY:
    ph = boost::shared_ptr<PHandleBase>(new PHandleFinDelay);
    stat_key = "findelay";
    break;


  default:
    log_->error("%s-->unknow msg type %d", fun, type);
    break;
  }

  if (ph) ph->process(this, sublayer_index, pb);

  long inv = tu.intv();
  if (inv > 1) {
    log_->info("%s-->tm:%ld t:%d", fun, inv, type);
  }

  callstat_fn(stat_key, inv, 0);

}

void LogicCore::from_sublayer_old(const string &sublayer_index, const string &pro)
{
  const char *fun = "LogicCore::from_sublayer";
  TimeUse tu;
  const char *stat_key = "unknown_proto";
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

    string cli_info;
    int rv = fin_fn(TIMEOUT_FIN, uid, pr, cli_info);
    if (rv) {
      // 
      if (1 == rv) {
        log_->info("%s-->nil online uid:%ld rv:%d head:%d conn:%lu tp:%d",
                  fun, uid, rv, head_len, conn_idx, pro_tp);
      } else {
        log_->warn("%s-->invalid uid:%ld rv:%d head:%d conn:%lu tp:%d",
                  fun, uid, rv, head_len, conn_idx, pro_tp);
      }
    } else {
      rv = offline_notify_fn(uid, cli_info);
      if (rv) {
        log_->error("%s-->offline_notify_fn uid:%ld rv:%d head:%d conn:%lu tp:%d",
                    fun, uid, rv, head_len, conn_idx, pro_tp);
      }
    }
    stat_key = "fin";

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

    stat_key = "findelay";
  } else if (PROTO_TYPE_UPIDX == pro_tp) {
    if (pro.size() != PROTO_LEN_UPIDX) {
      log_->info("%s->error upidx head:%d conn:%lu tp:%d", fun, head_len, conn_idx, pro_tp);
    }

    int sendidx = stream_ltt_bit32(&p, PROTO_LEN_SENDIDX);
    int recvidx = stream_ltt_bit32(&p, PROTO_LEN_RECVIDX);
    long uid = stream_ltt_bit64(&p, PROTO_LEN_UID);
    int expire = time_now_fn();
    int cli_tp = stream_ltt_bit32(&p, PROTO_LEN_CLI_TYPE);
    //log_->info("########### %d %d %ld", cli_tp, expire, uid);

    proto_upidx pr;
    pr.head.logic_conn = conn_idx;
    pr.head.idx.send_idx = sendidx;
    pr.head.idx.recv_idx = recvidx;
    pr.head.sublayer_index = sublayer_index;
    pr.expire = expire;
    pr.cli_type = cli_tp;

    proto_idx_pair ridx;
    int rv = upidx_fn(TIMEOUT_UPIDX, uid, pr, ridx);
    //    log_->info("##########-->rv:%d, sid:%d rid:%d", rv, ridx.send_idx, ridx.recv_idx);
    if (rv) {
      log_->error("%s-->uid:%ld rv:%d head:%d conn:%lu tp:%d",
                  fun, uid, rv, head_len, conn_idx, pro_tp);
    }

    stat_key = "upidx";

  }
  callstat_fn(stat_key, tu.intv(), 0);
}

int LogicCore::expire_stamp(int st, int cli_tp)
{
  
  switch (cli_tp) {

  case 0:
  case 1:
    return st + 60 * 30;

  case 100000:
    // use test
    return st + 20;

  case 200:
  default:
    return st + 60 * 10;

  }

}


void LogicCore::from_sublayer_synok(long uid, long conn, int cli_tp, const string &ver, const string &sublayer_index, const map<string, string> &data)
{
  const char *fun = "LogicCore::from_sublayer_synok";
  TimeUse tu;
  const char *stat_key = "synok";

  proto_syn p;
  p.head.logic_conn = conn;
  p.head.idx.send_idx = 1;
  p.head.sublayer_index = sublayer_index;

  int ns = time_now_fn();

  //  p.expire = expire_stamp(ns, cli_tp);
  p.expire = ns;

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

  callstat_fn(stat_key, tu.intv(), 0);
}


