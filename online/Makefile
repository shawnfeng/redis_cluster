include ../Makefile.mk

all: test_online test_ctrl

test_online: OnlineCtrl.o test.o
	$(CC) -o test_online test.o OnlineCtrl.o $(TEST_LINK_FLAG)

test_ctrl: test_ctrl.o
	$(CC) -o test_ctrl test_ctrl.o $(TEST_LINK_FLAG)

OnlineCtrl.o: OnlineCtrl.cpp OnlineCtrl.h
	$(CC) $(CCFLAGS) OnlineCtrl.cpp

test.o: test.cpp OnlineCtrl.h
	$(CC) $(CCFLAGS) test.cpp

test_ctrl.o: test_ctrl.cpp
	$(CC) $(CCFLAGS) test_ctrl.cpp




.PHONY: clean cleanall
clean:
	rm -f *.o
cleanall:
	rm -f *.o test_online test_ctrl