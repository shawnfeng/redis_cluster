#EVPATH=/home/code/redis_client/libev
#HIREDIS_PATH=/home/code/redis_client/hiredis
#ZKPATH=/home/code/rp/zookeeper/include

EVPATH=/data/home/guangxiang.feng/proj/trunk/talkplus/online_base/libev
HIREDIS_PATH=/data/home/guangxiang.feng/proj/trunk/talkplus/online_base/hiredis
ZKPATH=/data/home/guangxiang.feng/proj/xoa2_client/xoa2_env


# ============
CC=g++
#CCFLAGS=-c -Wall -g -fPIC -I$(EVPATH)/include -I$(HIREDIS_PATH) -I$(ZKPATH)/include -I../src
CCFLAGS=-c -Wall -g -fPIC -I$(EVPATH)/include -I$(HIREDIS_PATH) -I$(ZKPATH)/include
AR=ar
ARFLAGS=rcs

RPATH_EV=$(EVPATH)/lib
RPATH_HIREDIS=$(HIREDIS_PATH)
PATH_ZK=$(ZKPATH)/lib

LINK_FLAG=-lboost_thread -lcrypto
LINK_FLAG:=$(LINK_FLAG) -L$(RPATH_EV) -lev -Wl,-rpath,$(RPATH_EV)
LINK_FLAG:=$(LINK_FLAG) -L$(RPATH_HIREDIS) -lhiredis -Wl,-rpath,$(RPATH_HIREDIS)
LINK_FLAG:=$(LINK_FLAG) -L$(PATH_ZK) -lzookeeper_mt -Wl,-rpath,$(PATH_ZK)

TEST_LINK_FLAG:= -L../src -lredis_client $(LINK_FLAG)
