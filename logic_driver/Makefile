include ../Makefile.mk

OBJS=liblogic_driver.a

$(OBJS): LogicCore.o LogicConnLayerProto.pb.o PHandleSyn.o PHandleSynOk.o PHandleFin.o PHandleUpidx.o PHandleFinDelay.o
	$(AR) $(ARFLAGS) $@ $^

LogicCore.o: LogicCore.cpp LogicCore.h HookDef.h LogicConnLayerProto.pb.h PHandleBase.h
	$(CC) $(CCFLAGS) LogicCore.cpp

LogicConnLayerProto.pb.cc LogicConnLayerProto.pb.h: LogicConnLayerProto.proto
	protoc  -I=. --cpp_out=. $^

LogicConnLayerProto.pb.o: LogicConnLayerProto.pb.cc LogicConnLayerProto.pb.h
	$(CC) $(CCFLAGS) LogicConnLayerProto.pb.cc

PHandleSyn.o: PHandleSyn.cpp  PHandleBase.h
	$(CC) $(CCFLAGS) PHandleSyn.cpp

PHandleFin.o: PHandleFin.cpp  PHandleBase.h
	$(CC) $(CCFLAGS) PHandleFin.cpp

PHandleUpidx.o: PHandleUpidx.cpp  PHandleBase.h
	$(CC) $(CCFLAGS) PHandleUpidx.cpp


PHandleFinDelay.o: PHandleFinDelay.cpp  PHandleBase.h
	$(CC) $(CCFLAGS) PHandleFinDelay.cpp



.PHONY: clean cleanall
clean:
	rm -f *.o *.d *.d.*
cleanall:
	rm -f *.o *.d *.d.* $(OBJS)