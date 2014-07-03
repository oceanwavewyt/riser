riser(key/value)
==================
   riser wrap a network server for leveldb, and support for the memcache protocol.
      _               
 _ __(_)___  ___ _ __ 
| '__| / __|/ _ \ '__|
| |  | \__ \  __/ |   
|_|  |_|___/\___|_|   
                      
usage see --help

Dependencies:
	* libevent http://www.monkey.org/~provos/libevent/ (libevent-dev)
    * leveldb  https://code.google.com/p/leveldb/downloads/list

Environment:
	linux, Mac OS X

# Current version support for put, get 
# and del operation. You can config riser.conf 
# file for the riser server, i.e host and port 
# and so on. After starting the riser server, 
# you can access by telnet or the memcache 
# extension of php.

Build:
	make

Author:
	oceanwavewyt,  oceanwavewyt@gmail.com


