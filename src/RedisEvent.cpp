#include <string>

#include "RedisEvent.h"

using namespace std;

struct cmd_arg_t {
	boost::mutex mux;
	boost::condition_variable cond;

	RedisRvs *rv;
  map<uint64_t, RedisRv> err;
	cmd_arg_t(RedisRvs *r) : rv(r) {}
};

static void connect_cb(const redisAsyncContext *c, int status) {
	redisLibevEvents *re = (redisLibevEvents *)c->ev.data;
	//RedisEvent *rev = (RedisEvent *)re->data;

	//rcx->log()->info("connect_cb c:%p", c);

	userdata_t *u = (userdata_t *)ev_userdata(EV_DEFAULT);
	RedisEvent *rev = u->re();
	assert(rev);

	if (status != REDIS_OK) {
		rev->log()->error("connect_cb c:%p %s", c, c->errstr);
		//u->clear(re->addr);
		//return;
	} else {
		re->status = 1;
		rev->log()->info("connect_cb c:%p ok", c);
	}

}

static void disconnect_cb(const redisAsyncContext *c, int status) {


	//RedisEvent *rev = (RedisEvent *)re->data;
	userdata_t *u = (userdata_t *)ev_userdata(EV_DEFAULT);
	RedisEvent *rev = u->re();
	assert(rev);

	if (status != REDIS_OK) {
		rev->log()->error("disconnect_cb c:%p %s", c, c->errstr);
	} else {
		rev->log()->info("disconnect_cb c:%p ok", c);
	}
  /*
	redisLibevEvents *re = (redisLibevEvents *)c->ev.data;
  assert(re);
	u->clear(re->addr);

  free(re);

  redisAsyncContext *context = (redisAsyncContext *)c;
  context->ev.data = NULL;
  */
}


static void async_cb (EV_P_ ev_async *w, int revents)
{
	//	puts("async_cb just used for the side effects");
}


static void l_release (EV_P)
{
	userdata_t *u = (userdata_t *)ev_userdata (EV_A);
	u->re()->unlock();
}

static void l_acquire (EV_P)
{
	userdata_t *u = (userdata_t *)ev_userdata (EV_A);
	u->re()->lock();
}

static void redisrvs_add(redisReply *rp, RedisRv &rv)
{
	if (!rp) return;

  rv.type = rp->type;
  rv.integer = rp->integer;
  if (rp->str) rv.str.assign(rp->str, rp->len); 

  if (rp->type == REDIS_REPLY_ARRAY) {
    for (size_t i = 0; i < rp->elements; ++i) {
      RedisRv tmp_rv;
      redisrvs_add(rp->element[i], tmp_rv);
      rv.element.push_back(tmp_rv);
    }
  }

}

static void redisrp_redisrvs(redisReply *reply, cmd_arg_t &carg, uint64_t addr)
{
	if (!reply) return;

	RedisRvs &rvs = *carg.rv;
  map<uint64_t, RedisRv> &err = carg.err;

  if (reply->type != REDIS_REPLY_ERROR) {
    RedisRvs::iterator it =
      rvs.insert(pair< uint64_t, RedisRv >(addr, RedisRv())).first;
    redisrvs_add(reply, it->second);
  } else {
		RedisRv e;
    redisrvs_add(reply, e);
    err[addr] = e;
  }



}


static void redis_cmd_cb(redisAsyncContext *c, void *r, void *data)
{
	redisLibevEvents *re = (redisLibevEvents *)c->ev.data;
	// this point must alloc
	cflag_t *cf = (cflag_t *)data;
	assert(cf);
	userdata_t *u = (userdata_t *)ev_userdata(EV_DEFAULT);
	assert(u);
	assert(u->re());
	LogOut *log = u->re()->log();
	assert(log);

	log->trace("redis_cmd_cb-->cf=%p cf->f=%d cf->d=%p", cf, cf->f(), cf->d());

	if (cf->f() == 0 || cf->d() == NULL) {
		log->trace("redis_cmd_cb-->cf->f=%d return", cf->f());
		return;
	}


	// check ok, can use cf->d() pointer
	cmd_arg_t *carg = (cmd_arg_t *)cf->d();
	assert(carg);
  //	RedisRvs &rv = *carg->rv;
  //  map<uint64_t, RedisRv> &err = carg->err;


	if (cf->d_f() == 0) {
		log->trace("redis_cmd_cb-->reset 0 cf=%p cf->f=%d cf->d=%p", cf, cf->f(), cf->d());
		cf->reset();
	}

	redisReply *reply = (redisReply *)r;
	if (reply == NULL) {
		log->error("redis_cmd_cb-->reply null");
		goto cond;
	}

	log->trace("redis_cmd_cb-->type:%d inter=%lld len:%d argv:%s ele:%lu ep:%p",
		   reply->type, reply->integer, reply->len, reply->str, reply->elements, reply->element);

  redisrp_redisrvs(reply, *carg, re->addr);

	//sleep(2);
 cond:
	if (cf->f() == 0) {
		boost::mutex::scoped_lock lock(carg->mux);
		carg->cond.notify_one();
		log->trace("redis_cmd_cb-->over cmd and notify_one");
	}

}


void RedisEvent::run()
{
	struct ev_loop *loop = loop_;

	l_acquire (EV_A);
	ev_run (EV_A_ 0);
	l_release (EV_A);

}

void RedisEvent::start()
{
	log_->info("RedisEvent::start-->loop:%p", loop_);
	ev_async_init (&async_w_, async_cb);
	ev_async_start (loop_, &async_w_);

	//log_->info("now associate this with the loop");
	ev_set_userdata (loop_, &ud_);
	ev_set_loop_release_cb (loop_, l_release, l_acquire);

	//log_->info("then create the thread running ev_run");

	boost::thread td(boost::bind(&RedisEvent::run, this));
	td.detach();
	log_->info("RedisEvent::start-->ok loop:%p", loop_);

}

// caller had been locked
void RedisEvent::connect(uint64_t addr)
{
	const char *fun = "RedisEvent::connect";

	char ip[50];
	int port;
	int64_ipv4(addr, ip, sizeof(ip), port);

	//	log_->info("%s-->connect ip:%s port:%d", ip, port);
	redisAsyncContext *c = redisAsyncConnect(ip, port);
	log_->info("%s-->connect ip:%s port:%d c:%p", fun, ip, port, c);
	if (c->err) {
		log_->error("%s-->%s %p", fun, c->errstr, c);
		return;
	}
	c->ev.data = NULL;

	//ud_.insert(addr, c);

	redisLibevAttach(loop_, c, addr);
	redisAsyncSetConnectCallback(c, connect_cb);
	redisAsyncSetDisconnectCallback(c, disconnect_cb);

}

void RedisEvent::cmd(RedisRvs &rv,
                     const char *log_key,
                     const map< uint64_t, vector<string> > &addr_cmd,
                     int timeout,
                     const string &lua_code, bool iseval
                     )
{
	//userdata *u = (userdata *)ev_userdata (loop_);
  TimeUse tu;
	const char *fun = "RedisEvent::cmd";
	ReqCount rcount(req_count_);

	log_->debug("%s-->k:%s size:%lu rcount=%d", fun, log_key, addr_cmd.size(), rcount.cn());
	if (addr_cmd.empty()) {
		log_->warn("%s-->k:%s empty redis context", fun, log_key);
		return;
	}


	userdata_t *u = &ud_;

	cmd_arg_t carg(&rv);
	cflag_t *cf = NULL;


	// ==========lock==========
	log_->trace("%s-->loop lock", fun);
	mutex_.lock();


	cf = u->get_cf();
	if (cf->f() || cf->d()) {
		log_->error("%s-->userdata have data!", fun);
	}

	int wsz = 0;
	for (map< uint64_t, vector<string> >::const_iterator it = addr_cmd.begin();
	     it != addr_cmd.end(); ++it) {

    const vector<string> &args = it->second;
    size_t argc = args.size();
    // copy the args
    if (argc >= ARGV_MAX_LEN) {
      log_->error("%s-->args more size:%lu", fun, args.size());
      continue;
    }

    const char **argv = cmd_argv_;
    size_t *argvlen = cmd_argvlen_;
    for (size_t i = 0; i < args.size(); ++i) {
      argv[i] = args[i].c_str();
      argvlen[i] = args[i].size();
    }

    if (iseval) {
      argv[0] = "EVAL";
      argv[1] = lua_code.c_str();
      if (argvlen) {
        argvlen[0] = 4;
        argvlen[1] = lua_code.size();
      }
    }
    //----------------------

    const uint64_t &ad = it->first;
		redisAsyncContext *c = u->lookup(ad);
		if (NULL == c) {
      log_->warn("%s-->add connection addr:%lu", fun, ad);
			connect(ad);
		} else {
			redisLibevEvents *rd = (redisLibevEvents *)c->ev.data;
			assert(rd);
			log_->trace("%s-->c:%p e:%p st:%d", fun, ad, rd, rd->status);
			if (1 == rd->status) {				
				if (REDIS_ERR == redisAsyncCommandArgv(c, redis_cmd_cb, cf, argc, argv, argvlen)) {
          log_->error("%s-->redisAsyncCommandArgv %s %p", fun, c->errstr, c);
        } else {
          wsz++;
        }
			} else {
				log_->warn("%s-->connection is not ready c:%p", fun, c);
			}

		}

	}
	cf->set_f(wsz);
	cf->set_d((void *)&carg);


	ev_async_send (loop_, &async_w_);

	bool is_timeout = true;
	{
		// waiting
		boost::mutex::scoped_lock lock(carg.mux);  // must can get lock!

	mutex_.unlock(); // card.mux had beed locked, then release mutex_
	log_->trace("%s-->loop unlock", fun);
	// ==========unlock==========
	        log_->trace("%s-->condition wait %d", fun, timeout);
		is_timeout = !carg.cond.timed_wait(lock, boost::posix_time::milliseconds(timeout));
	}

	log_->trace("%s-->condition pass istimeout=%d", fun, is_timeout);
	if (is_timeout) {
		boost::mutex::scoped_lock lock(mutex_);
		log_->trace("%s-->cf=%p cf->f=%d cf->d=%p", fun, cf, cf->f(), cf->d());
		cf->reset();
	}
	// log out free lock
	if (is_timeout) {
		log_->warn("%s-->k:%s timeout format", fun, log_key);
	}




  // first load script
  if (
      !lua_code.empty()
      && !carg.err.empty()
      ) {

    map< uint64_t, vector<string> > erraddrs;
    for (map<uint64_t, RedisRv>::const_iterator it = carg.err.begin();
         it != carg.err.end();
         ++it) {
      
      map< uint64_t, vector<string> >::const_iterator jt = addr_cmd.find(it->first);
      assert(jt != addr_cmd.end());

      const vector<string> &args = jt->second;
      size_t argc = args.size();
      if (argc >=2
          // sorry! EVALSHA commend must be used caps
          && args.at(0) == "EVALSHA"
          && !strncmp(it->second.str.c_str(), "NOSCRIPT", 8)) {

        log_->warn("%s-->%lu first load  script:%s", fun, it->first, lua_code.c_str());
        erraddrs[it->first] = args;

      }

    }
    
    cmd(rv, log_key, erraddrs, timeout, lua_code, true);
  }

  // error log, use the caller thread print
  for (map<uint64_t, RedisRv>::const_iterator it = carg.err.begin();
       it != carg.err.end();
       ++it) {
    log_->error("%s-->addr:%lu err:%s", fun, it->first, it->second.str.c_str());
  }

	log_->info("%s-->k:%s size:%lu wsz:%d istimeout:%d rcount:%d tm:%ld",
             fun, log_key, addr_cmd.size(), wsz, is_timeout, rcount.cn(), tu.intv());

}


// =============================
cflag_t *userdata_t::get_cf()
{
  if (++idx_ >= CUR_CALL_NUM) idx_ = 0;

  re_->log()->trace("userdata_t::get_cf idx:%lu", idx_);

  cflag_t *cf = &cfg_[idx_];
  return cf;
}

void userdata_t::insert(uint64_t addr, redisAsyncContext *c)
{
	assert(ctxs_.find(addr) == ctxs_.end());
	re_->log()->info("userdata_t::insert addr:%lu c:%p", addr, c);
	ctxs_[addr] = c;
}

void userdata_t::clear(uint64_t addr)
{
	std::map<uint64_t, redisAsyncContext *>::iterator it = ctxs_.find(addr);
	assert(it != ctxs_.end());
  if (it == ctxs_.end()) {
    re_->log()->error("userdata_t::clear why empty clear addr:%lu", addr);
    return;
  }

	re_->log()->info("userdata_t::clear addr:%lu c:%p", addr, it->second);
	ctxs_.erase(it);
}

redisAsyncContext *userdata_t::lookup(uint64_t addr)
{
	std::map<uint64_t, redisAsyncContext *>::const_iterator it = ctxs_.find(addr);
	if (it == ctxs_.end()) {
		return NULL;
	} else {
		return it->second;
	}
}

//==========
void RedisRv::dump(LogOut *logout, const char *pref, int lv, int depth) const
{
  const char *fun = "RedisRv::dump";
  void (LogOut::*log)(const char *format, ...);
  log = &LogOut::trace;

  switch (lv) {
  case 0:
      log = &LogOut::trace;
      break;
  case 1:
      log = &LogOut::debug;
      break;
  case 2:
      log = &LogOut::info;
      break;
  case 3:
      log = &LogOut::warn;
      break;
  case 4:
      log = &LogOut::error;
      break;
  default:
    break;

  }

  string log_depth;
  log_depth.assign(depth*2, '*');

  (logout->*log)("%s-->%s %s type:%d intger:%d str:%s ele:%lu",
                 fun, pref, log_depth.c_str(), type, integer, str.c_str(), element.size());

  if (type == REDIS_REPLY_ARRAY) {
    for (vector<RedisRv>::const_iterator it = element.begin();
         it != element.end(); ++it) {
      it->dump(logout, pref, lv, depth+1);
    }
  }

}
