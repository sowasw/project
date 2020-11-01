ROOT=../../
include $(ROOT)Make.defines

PROG=server/weBayServer	\
	client/weBayClient

all:$(PROG) 
#	cd sql_scripts && ./init_webay_db.sh
#	redis-server redis.conf &
	
server/weBayServer: weBayServer.o weBay.o $(OBJS)
	mkdir -p server
	$(CC) $^  $(INCLUDEOPTS) $(LIBOPTS) -o $@ -g

client/weBayClient:weBayClient.o weBay.o $(OBJS)
	mkdir -p client
	$(CC) $^ $(INCLUDEOPTS) $(LIBOPTS) -o $@

	
%.o: %.cpp $(OBJS)
	$(CC) -c $< $(INCLUDEOPTS) -o $@
	
.PHONY:clean
clean:
	rm -f $(PROG) *.o a.out 
	
include $(ROOT)Make.objs.inc

