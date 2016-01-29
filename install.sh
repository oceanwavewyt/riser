libpath="./lib/"
if [ ! -d $libpath ]; then 
	mkdir $libpath
fi
cd pack/libevent/
./configure  && make && make install
cd ../linenoise/ && make
cp linenoise.h ../../include/
cd ../leveldb/
make && cp libleveldb.* ../../$libpath/
cd ../hiredis/
make && cp libhiredis.a ../../$libpath/
cp *.h ../../include/  
cd ../../
make
if [ $# -ge 1 ]; then
    path="$1"
	if [ ! -d $path ]; then
		mkdir -p $path
		if [ ! -d $path ]; then
			echo "create $path failed"
		else
			echo "create $path ok"
		fi
	fi
	curpath=$(cd "$(dirname "$0")"; pwd)		
	cd $path
	cp -r $curpath/lib  ./
	test -d conf || mkdir  conf/
	test -d log  || mkdir  log/
	test -f conf/riser.conf || cp  $curpath/riser.conf  conf/
	cp $curpath/riser-server  ./
fi
