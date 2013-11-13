include ../Makefile.mk

#SRCS=libev.cpp  RedisClient.cpp  RedisEvent.cpp  RedisHash.cpp  Util.cpp
SRCS=$(wildcard *.cpp)

OBJS=libredis_client.a

$(OBJS): $(SRCS:.cpp=.o)
	$(AR) $(ARFLAGS) $@ $^


include $(SRCS:.cpp=.d)


%.d: %.cpp
	set -e; rm -f $@; \
	$(CC) -MM $(CCFLAGS) $< > $@.$$$$; \
	echo "	\$$(CC) \$$(CCFLAGS) $<" >> $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$




.PHONY: clean cleanall
clean:
	rm -f *.o *.d *.d.*
cleanall:
	rm -f *.o *.d *.d.* $(OBJS)