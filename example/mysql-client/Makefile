ROOT=../../
include $(ROOT)Make.defines

PROG=mysql-client

all:$(PROG)

mysql-client: mysql-client.o $(OBJS)
	$(CC) mysql-client.o  $(OBJS)  $(INCLUDEOPTS) $(LIBOPTS) -o mysql-client
	
%.o: %.cpp 
	$(CC) -c $< $(INCLUDEOPTS) -o $@
	
.PHONY:clean
clean:
	rm -f mysql-client *.o a.out 
	
include $(ROOT)Make.objs.inc
