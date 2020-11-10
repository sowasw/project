ROOT=../../
include $(ROOT)Make.defines

PROGS = wx_redis-lock-test \
	wx_redis-pipeline-test	

TEMPOBJ = /tmp/wx_redis.o
	

all:$(PROGS) $(TEMPOBJ)

%.o: %.cpp 
	$(CC) -c $< $(INCLUDEOPTS)  -o $@

$(TEMPOBJ):
	sed -n '/REDIS_LOCK_DEBUG/s/\/\/#define/#define/; w /tmp/wx_redis.h' $(ROOT)src/wx_redis.h
	cp $(ROOT)src/wx_redis.cpp /tmp/wx_redis.cpp
	$(CC) -c /tmp/wx_redis.cpp -I/tmp $(INCLUDEOPTS) -lhiredis -o $@ 

wx_redis-lock-test: wx_redis-lock-test.o  $(TEMPOBJ) $(ROOT)obj/wx_lib.o
	g++ $^  -I/tmp  $(INCLUDEOPTS) -lhiredis -o $@ 
#	g++ wx_redis-lock-test.cpp $(ROOT)src/wx_redis.cpp  $(ROOT)src/wx_lib.cpp  $(INCLUDEOPTS) -lhiredis -o $@ 

wx_redis-pipeline-test: wx_redis-pipeline-test.o  $(ROOT)obj/wx_redis.o $(ROOT)obj/wx_lib.o
	g++ $^  $(INCLUDEOPTS) -lhiredis -o $@ 
	
.PHONY:clean	
clean: 
	rm -f a.out *.o $(PROGS) $(TEMPOBJ)
	
include $(ROOT)Make.objs.inc
