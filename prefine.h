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
//#include <types.h>
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
using namespace std;

#define LOGBUFSIZE	2048
#define LINEBUFSIZE	4096
//LOG



struct GentLog
{
public:
	enum logLevel {INFO=0,WARN=1,ERROR=2,FATAL=3};
	static void write(int levels, const char *file, const int line, const char *func, const char *format, ...);
};

#define LOG(level, args...)  GentLog::write(level, __FILE__, __LINE__, __func__, args)




#endif /* PREFINE_H_ */
