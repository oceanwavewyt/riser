riser(key/value)
==================
       riser wrap a network server for leveldb, and support for the memcache protocol.
	      _               
	 _ __(_)___  ___ _ __ 
	| '__| / __|/ _ \ '__|
	| |  | \__ \  __/ |   
	|_|  |_|___/\___|_|   

--------------	                      
usage see --help

### Dependencies:
    * libevent http://www.monkey.org/~provos/libevent/ (libevent-dev)
    * leveldb  https://code.google.com/p/leveldb/downloads/list

### Environment:
     linux, Mac OS X

### Instruction
  Current version support for put, get 
  and del operation. You can config riser.conf 
  file for the riser server, i.e host and port 
  and so on. After starting the riser server, 
  you can access by telnet or the memcache,redis  
  extension of php.

### Build:
    ./install.sh

### config file
    riser.conf
    
### client
     redis's method: set(key, value),setex(key,value,time),get(key),mget(array),delete(key),keys,exists(key),
     close(),ttl(key),auth(string)
     
     memcache's method:  set(key, value), get(key), get(array), delete(key), close()
     
### redis command line
     set, get,del, keys, exists, info, ping
     其中 info rep  命令查看slave状况   

### replicaton
     开启配置文件riser.conf中，
     #slaveof_ip=127.0.0.1
     #slaveof_port=3555
     
### Author:
    oceanwavewyt,  oceanwavewyt@gmail.com


