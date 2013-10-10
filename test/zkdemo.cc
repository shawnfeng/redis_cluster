#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <string>
#include <vector>
#include <algorithm>

#include <zookeeper/zookeeper.h>

#include "Util.h"


using namespace std;

static LogOut g_log;

struct string_cb_data {
	zhandle_t *zh;
	const char *path;
	const char *info;
	//	void *watcher;
};

static void string_cb(int rc,
		      const struct String_vector *strings, const void *data);

static void init_wc(zhandle_t *zh, int type, int state, const char *path,
             void *watcherCtx)
{
	g_log.trace("init_wc-->zh=%p,type=%d,state=%d,path=%s,watcherCtx=%p:%s",
		    zh, type, state, path, watcherCtx, (char *)watcherCtx);
}

static void child_wc(zhandle_t *zh, int type, int state, const char *path,
             void *watcherCtx)
{
	g_log.trace("chile_wc-->zh=%p,type=%d,state=%d,path=%s,watcherCtx=%p",
		    zh, type, state, path, watcherCtx);


	g_log.trace("child_wc-->readd watcher");
	zoo_awget_children(zh,
			   path,
			   child_wc, watcherCtx,
			   string_cb, watcherCtx);


}

static void string_cb(int rc,
        const struct String_vector *strings, const void *data)
{

	string_cb_data *sd = (string_cb_data *)data;
	g_log.trace("string_cb-->rc=%d,strings=%p,data=%p:%s",
		    rc, strings, data, sd->info
		    );

	if (ZOK != rc) {
		// 节点不存在时候，后续即使节点被创建了也不会触发watcher
		// 所以要再设置一下
		g_log.error("string_cb-->node is not exist");
		zoo_awget_children(sd->zh,
				   sd->path,
				   child_wc, (void *)data,
				   string_cb, data);

		sleep(2);

	}

	if (ZOK == rc) {
		char **dp = strings->data;
		for (int i = 0; i < strings->count; ++i) {
			g_log.info("%s/%s", sd->path, *dp++);
		}

	}


}


int main(int argc, char **argv) {
	int rc;

	zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);

	const char *zkhost = "127.0.0.1:4180,127.0.0.1:4181,127.0.0.1:4182";

	zhandle_t *zh = zookeeper_init(zkhost, init_wc, 10000, 0, (void *)"init zk", 0);

	g_log.info("main-->zkhost=%s,zh=%p", zkhost, zh);

	if (!zh) {
		g_log.error("ERROR: zk init error!");
		return 1;
	}

	const char *path = "/test";

	string_cb_data sd;
	sd.zh = zh;
	sd.info = "strings_completion_t data";
	sd.path = path;
	//	sd.watcher = (void *)&sd;
	rc = zoo_awget_children(zh,
				path,
				child_wc, (void *)&sd,
				string_cb, (void *)&sd);


	if (rc != ZOK) {
		g_log.error("ERROR: zk awget children error %d!", rc);
		return 1;
	}


	g_log.info("main-->hold here");
	pause();



	zookeeper_close(zh);
	return 0;
}

