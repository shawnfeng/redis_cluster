include ../Makefile.mk

test_online: OnlineCtrl.o test.o
	$(CC) -o test_online test.o OnlineCtrl.o $(TEST_LINK_FLAG)


OnlineCtrl.o: OnlineCtrl.cpp OnlineCtrl.h
	$(CC) $(CCFLAGS) OnlineCtrl.cpp

test.o: test.cpp OnlineCtrl.h
	$(CC) $(CCFLAGS) test.cpp




.PHONY: clean cleanall
clean:
	rm -f *.o
cleanall:
	rm -f *.o test_online