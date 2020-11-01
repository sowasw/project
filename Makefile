exampledirs=example/echo-server example/mysql-client example/redis-test \
	example/weBay

all:
	for dir in $(exampledirs); do	\
		(cd $$dir && make)	\
	done	
	
clean:
	for dir in $(exampledirs); do	\
		(cd $$dir && make clean)	\
	done
	(cd obj && make clean)
