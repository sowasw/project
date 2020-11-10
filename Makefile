projectsdirs=projects/echo-server projects/mysql-client projects/redis-test \
	projects/weBay

all:
	for dir in $(projectsdirs); do	\
		(cd $$dir && make)	\
	done	
	
clean:
	for dir in $(projectsdirs); do	\
		(cd $$dir && make clean)	\
	done
	(cd obj && make clean)
