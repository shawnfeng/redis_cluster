#include "PHandleBase.h"

void PHandleSyn::process(LogicCore *lc, const std::string &sublayer_index, const ProHead &ph)
{

  const char *fun = "PHandleSyn::process";
  lc->log()->info("%s-->%s %s", fun, sublayer_index.c_str(), ph.DebugString().c_str());

  ProSyn ps;
  if (!ps.ParseFromString(ph.sub_pro())) {
    lc->log()->error("%s-->error parser %s", fun, sublayer_index.c_str());
  }

  lc->log()->info("%s-->%s %s", fun, sublayer_index.c_str(), ps.DebugString().c_str());

  
  //  lc->callstat_fn("syn", tu.intv(), 0);
}


