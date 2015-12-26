/*
 * prefine.cpp
 *
 *  Created on: 2012-3-8
 *      Author: wyt
 */
#include "prefine.h"
static const char *levelname[] = {"BUG","INFO","WARN", "ERROR", "FATAL"};
FILE *GentLog::logfd = stdout;
int GentLog::runLevel = GentLog::INFO;

int GentLog::setfd(string &filename)
{
	if(filename == "") return 1;
	if((logfd = fopen(filename.c_str(), "a+")) == NULL) {
		return 0;
	}	
	return 1;
}

void GentLog::setLevel(string &loglevel)
{
	if(loglevel == "bug" || loglevel == "BUG"){
		GentLog::runLevel = GentLog::BUG;
	}else if(loglevel == "info" || loglevel == "INFO"){
		GentLog::runLevel = GentLog::INFO;
	}else if(loglevel == "warn" || loglevel == "WARN"){
		GentLog::runLevel = GentLog::WARN;
	}else if(loglevel == "error" || loglevel == "ERROR"){ 
		GentLog::runLevel = GentLog::ERROR;
	}else if(loglevel == "fatal" || loglevel == "FATAL"){
		GentLog::runLevel = GentLog::FATAL;
	}
}

void GentLog::write(int levels, const char *file, const int line, const char *func, const char *format, ...)
{
	if(levels < GentLog::runLevel) return;
	char buf[LOGBUFSIZE];                  
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, LOGBUFSIZE, format, ap);
	va_end(ap);	
	
	time_t now;
	struct tm *tm;
	time(&now);	
	tm = localtime(&now);
	char str[LINEBUFSIZE];
#ifdef _DARWIN                                                   
	snprintf(str, LINEBUFSIZE, "%s [%d-%02d-%02d %02d:%02d:%02d] [%lu] %s", levelname[levels], tm->tm_year+1900,tm->tm_mon,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec, pthread_self()->__sig, buf);
#else
	snprintf(str, LINEBUFSIZE, "%s [%d-%02d-%02d %02d:%02d:%02d] [%lu] %s", levelname[levels], tm->tm_year+1900,tm->tm_mon,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec, pthread_self(), buf);	
#endif
	fprintf(logfd, "%s\n", str);                                                                  	
	fflush(logfd);
}

void GentLog::console(int level, const string &str)
{
	cout << levelname[level] << " "<<str<<endl;
}
