libpath="./lib/"
if [ ! -d $libpath ]; then 
	mkdir $libpath
fi
cd pack/libevent/
./configure  && make && make install
cd ../leveldb/
make && cp libleveldb.* ../../$libpath/
cd ../../
make
