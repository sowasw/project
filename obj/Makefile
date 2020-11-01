ROOT=../
include ../Make.defines

all:$(OBJS)

$(ROOT)obj/EventLoop.o: $(SRCPATH)EventLoop.cpp $(SRCPATH)EventLoop.h
	$(CC) -c $< -o $@ $(LIBOPTS)

$(ROOT)obj/Log.o: $(SRCPATH)Log.cpp $(SRCPATH)Log.h
	$(CC) -c $< -o $@ $(LIBOPTS)
	
$(ROOT)obj/TcpServer.o: $(SRCPATH)TcpServer.cpp $(SRCPATH)TcpServer.h
	$(CC) -c $< -o $@ $(LIBOPTS)
	
$(ROOT)obj/Thread.o: $(SRCPATH)Thread.cpp $(SRCPATH)Thread.h
	$(CC) -c $< -o $@ $(LIBOPTS)
	
$(ROOT)obj/wx_mysql.o: $(SRCPATH)wx_mysql.cpp $(SRCPATH)wx_mysql.h
	$(CC) -c $< -o $@ $(LIBOPTS) -I$(INCLUDEPATH)
	
$(ROOT)obj/wx_redis.o: $(SRCPATH)wx_redis.cpp $(SRCPATH)wx_redis.h
	$(CC) -c $< -o $@ $(LIBOPTS) $(INCLUDEOPTS)
	
$(ROOT)obj/wx_lib.o: $(SRCPATH)wx_lib.cpp $(SRCPATH)wx_lib.h
	$(CC) -c $< -o $@ $(LIBOPTS) $(INCLUDEOPTS)
	
$(ROOT)obj/Protocol.o: $(SRCPATH)Protocol.cpp $(SRCPATH)Protocol.h
	$(CC) -c $< -o $@ $(INCLUDEOPTS)
	
.PHONY: clean
clean:
	rm -f $(OBJS)


