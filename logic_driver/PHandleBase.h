#ifndef __PHANDLE_BASE_H_H__
#define __PHANDLE_BASE_H_H__

#include "LogicCore.h"
#include "LogicConnLayerProto.pb.h"

class PHandleBase {

 public:
  virtual ~PHandleBase() {}
  virtual void process(LogicCore *lc, const std::string &sublayer_index, const ProHead &ph) = 0;

};


class PHandleSyn : public PHandleBase {
 public:
  virtual void process(LogicCore *lc, const std::string &sublayer_index, const ProHead &ph);
};


class PHandleSynOk : public PHandleBase {
 public:
  virtual void process(LogicCore *lc, const std::string &sublayer_index, const ProHead &ph);
};

class PHandleFin : public PHandleBase {
 public:
  virtual void process(LogicCore *lc, const std::string &sublayer_index, const ProHead &ph);
};

class PHandleUpidx : public PHandleBase {
 public:
  virtual void process(LogicCore *lc, const std::string &sublayer_index, const ProHead &ph);
};

class PHandleFinDelay : public PHandleBase {
 public:
  virtual void process(LogicCore *lc, const std::string &sublayer_index, const ProHead &ph);
};


#endif
