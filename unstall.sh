libpath="./lib/"
rm -f $libpath*
cd pack/libevent/
make clean
cd ../leveldb/
make clean
cd ../../
make clean
