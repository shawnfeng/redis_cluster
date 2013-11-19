include ../Makefile.mk

test: test_client
# 1st zkdemo

1st: 1st.o
	$(CC) -o 1st -Wl,-rpath $(RPATH_EV) 1st.o -L$(RPATH_EV) -lev

# -Wl,-rpath path可以用: 分隔
example-libev: example-libev.o
	$(CC) -o example-libev -Wl,-rpath,$(RPATH_EV) -Wl,-rpath,$(RPATH_HIREDIS) example-libev.o -L$(RPATH_HIREDIS) -L$(RPATH_EV) -lev -lhiredis

demo: demo.o
	$(CC) -o demo -Wl,-rpath,$(RPATH_EV) -Wl,-rpath,$(RPATH_HIREDIS) demo.o -L$(RPATH_HIREDIS) -L$(RPATH_EV) -lev -lhiredis -lpthread

zkdemo: zkdemo.o
	$(CC) -o zkdemo zkdemo.o $(TEST_LINK_FLAG)


1st.o: 1st.c
	$(CC) $(CCFLAGS) 1st.c

example-libev.o: example-libev.cc
	$(CC) $(CCFLAGS) example-libev.cc

demo.o: demo.cc demo.h
	$(CC) $(CCFLAGS) demo.cc

zkdemo.o: zkdemo.cc
	$(CC) $(CCFLAGS) zkdemo.cc


# src client
test_client: test_client.o
	$(CC) -o test_client test_client.o $(TEST_LINK_FLAG)

test_client.o: test_client.cpp
	$(CC) $(CCFLAGS) test_client.cpp


.PHONY: clean cleanall
clean:
	rm -f *.o
cleanall:
	rm -f *.o 1st example-libev demo test_client zkdemo