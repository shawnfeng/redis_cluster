#ifndef __HOOK_DEFINE_H_H__
#define __HOOK_DEFINE_H_H__
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>

struct proto_idx_pair {
  int send_idx;
  int recv_idx;
proto_idx_pair() : send_idx(0), recv_idx(0) {}

  int keys(std::vector<std::string> &d) const
  {
    d.push_back(boost::lexical_cast<std::string>(send_idx));
    d.push_back(boost::lexical_cast<std::string>(recv_idx));
    return (int)d.size();
  }
};

struct proto_gb_head {
  long logic_conn;
  proto_idx_pair idx;
  std::string sublayer_index;

  int keys(std::vector<std::string> &d) const
  {
    d.push_back(boost::lexical_cast<std::string>(logic_conn));
    idx.keys(d);
    d.push_back(sublayer_index);
    return (int)d.size();
  }

};

struct proto_syn {
  proto_gb_head head;
  int expire;

  int cli_type;
  std::string cli_ver;
  std::vector<std::string> data;

proto_syn() : expire(0), cli_type(-1) {}

  int keys(std::vector<std::string> &d) const
  {
    head.keys(d);
    d.push_back(boost::lexical_cast<std::string>(expire));

    d.push_back(boost::lexical_cast<std::string>(cli_type));
    d.push_back(cli_ver);
    return (int)d.size();
  }

};

struct proto_fin {
  proto_gb_head head;
  int keys(std::vector<std::string> &d) const
  {
    head.keys(d);
    return (int)d.size();
  }
};

struct proto_fin_delay {
  proto_gb_head head;
  int expire;
  int keys(std::vector<std::string> &d) const
  {
    head.keys(d);
    d.push_back(boost::lexical_cast<std::string>(expire));
    return (int)d.size();
  }
};


struct proto_upidx {
  proto_gb_head head;
  int expire;
proto_upidx() : expire(0) {}
  int keys(std::vector<std::string> &d) const
  {
    head.keys(d);
    d.push_back(boost::lexical_cast<std::string>(expire));
    return (int)d.size();
  }

};

struct proto_ack {
  proto_gb_head head;
  int keys(std::vector<std::string> &d) const
  {
    head.keys(d);
    return (int)d.size();
  }

};

typedef int (*hook_syn_fn)(int timeout, long uid, const proto_syn &proto, proto_idx_pair &idx);
typedef int (*hook_fin_fn)(int timeout, long uid, const proto_fin &proto);
typedef int (*hook_fin_delay_fn)(int timeout, long uid, const proto_fin_delay &proto);
typedef int (*hook_upidx_fn)(int timeout, long uid, const proto_upidx &proto);
typedef int (*hook_timeout_rm_fn)(int timeout, int stamp, int count);


#endif
