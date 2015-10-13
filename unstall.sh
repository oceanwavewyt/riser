libpath="./lib/"
rm -f $libpath*
cd pack/libevent/
make clean
cd ../leveldb/
make clean
cd ../../
rm -f gent_hw.h
make clean
