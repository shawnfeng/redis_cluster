#ifndef __ZOOKEEPER_INIT_H_H__
#define __ZOOKEEPER_INIT_H_H__
class ZKinit {
  LogOut *log_;
  zhandle_t *zkh_;

static void init_watcher(zhandle_t *zh, int type, int state, const char *path,
                         void *data)
  {

    LogOut *log = (LogOut *)data;

    log->info("init_wc-->zh=%p,type=%d,state=%d,path=%s",
              zh, type, state, path);
  }

public:
  ZKinit(LogOut *log, const char *zk_addr) : log_(log), zkh_(NULL)
  {
    const char *fun = "ZKinit::ZKinit";
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    zkh_ = zookeeper_init(zk_addr, init_watcher, 10000, 0, log_, 0);
    if (!zkh_) {
      log_->error("%s-->error init zk %s zk can not used, please check add restart", fun, zk_addr);

    }

    log_->info("%s-->init ok zk %s", fun, zk_addr);

  }

  zhandle_t *handle() { return zkh_; }
};



#endif
