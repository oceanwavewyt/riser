/*
 * prefine.h
 *
 *  Created on: 2012-3-8
 *      Author: wyt
 */

#ifndef PREFINE_H_
#define PREFINE_H_

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include<ctype.h>
#include <pthread.h>
#include <assert.h>
#include <map>
#include <iostream>
#include <vector>
#include <list>

#include "gent_hw.h"
typedef unsigned char byte;
using namespace std;

const uint32_t LOGBUFSIZE = 2048;
const uint32_t LINEBUFSIZE = 4096;
//单位M
const int TARGET_FILE_SIZE = 128;
//单位M
const int WRITE_BUFFER_SIZE = 32;
//从服务器的名字长度
const int SLAVE_NAME_SIZE = 50;
//从服务器数量最多1024
const int SLAVE_NUM = 1024;
//队列最大链接数
const int MAX_CONNECT = 10000;
//LOG
struct GentLog
{
public:
	enum logLevel {BUG=0,INFO=1,WARN=2,ERROR=3,FATAL=4 };
	static int runLevel;
	static FILE *logfd;
	static string logfile;
	static bool isServer;
	static int setfd(string &filename);
	static void setServer(bool s);
	static void setLevel(string &loglevel);
	static void write(int levels, const char *file, const int line, const char *func, const char *format, ...);
	static void console(int level, const string &);
};

class CommLock
{
private:
    pthread_mutex_t _lock;
public:
    CommLock()
	{
		pthread_mutex_init(&_lock,NULL);
	}
	~CommLock(){}
	void Lock()
	{
		pthread_mutex_lock(&_lock);
	}
	void UnLock()
	{
		pthread_mutex_unlock(&_lock);
	}
};

class AutoLock{
	CommLock* _lock;
public:
	AutoLock(CommLock * lock)
	{
		_lock = lock;
		_lock->Lock();
	}
    
    ~AutoLock()
	{
		_lock->UnLock();
	}
};

struct riserserver
{
	int port;
	char *configfile;
	char server_id[100];
};

#define LOG(level, args...)  GentLog::write(level, __FILE__, __LINE__, __func__, args)
#define INFO(level,str) GentLog::console(level,str)

#define REGISTER_COMMAND(p, LogicModule) \
			p = new LogicModule(); \
			GentAppMgr::Instance()->SetPlugin(p); 


#endif /* PREFINE_H_ */
