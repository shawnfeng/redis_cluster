#ifndef __ONLINE_PROTO_H_H__
#define __ONLINE_PROTO_H_H__
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

struct proto_online_head {
  std::string logic_conn;
  proto_idx_pair idx;

  int keys(std::vector<std::string> &d) const
  {
    d.push_back(logic_conn);
    idx.keys(d);
    return (int)d.size();
  }

};

struct proto_syn {
  proto_online_head head;
  int expire;
  std::string gate;

  int cli_type;
  //  std::string cli_ver;
  std::vector<std::string> data;

proto_syn() : expire(0), cli_type(-1) {}

  int keys(std::vector<std::string> &d) const
  {
    head.keys(d);
    d.push_back(boost::lexical_cast<std::string>(expire));
    d.push_back(gate);

    d.push_back(boost::lexical_cast<std::string>(cli_type));
    //    d.push_back(cli_ver);
    return (int)d.size();
  }

};

struct proto_fin {
  proto_online_head head;
  int keys(std::vector<std::string> &d) const
  {
    head.keys(d);
    return (int)d.size();
  }
};

struct proto_heart {
  proto_online_head head;
  int expire;
proto_heart() : expire(0) {}
  int keys(std::vector<std::string> &d) const
  {
    head.keys(d);
    d.push_back(boost::lexical_cast<std::string>(expire));
    return (int)d.size();
  }

};

struct proto_ack {
  proto_online_head head;
  int keys(std::vector<std::string> &d) const
  {
    head.keys(d);
    return (int)d.size();
  }

};



#endif
