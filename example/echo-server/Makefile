ROOT = ../../

#预定义变量的文件
include $(ROOT)Make.defines

PROGS = EchoServer1	\
	EchoServer2	\
	EchoServer3


all:$(PROGS)

# 把每个cpp文件编译成obj目标文件
%.o: %.cpp 
	$(CC) -c $< $(INCLUDEOPTS) -o $@

#eventLoopTest.o:eventLoopTest.cpp 
#	$(CC) -c -o $@ $< -I$(SRCPATH)
	
EchoServer1:EchoServer1.o $(OBJS)
	$(CC) -o $@ $^ $(INCLUDEOPTS) $(LIBOPTS)
	
EchoServer2:EchoServer2.o $(OBJS)
	$(CC) -o $@ $^ $(INCLUDEOPTS) $(LIBOPTS)
	
EchoServer3:EchoServer3.o $(OBJS)
	$(CC) -o $@ $^ $(INCLUDEOPTS) $(LIBOPTS)

.PHONY:clean
clean:
	rm -f a.out *.o $(PROGS)

# 此文件在root/obj目录下执行make，生成依赖目标$(OBJS)	
include $(ROOT)Make.objs.inc
