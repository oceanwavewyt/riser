/*
 * prefine.cpp
 *
 *  Created on: 2012-3-8
 *      Author: wyt
 */
#include "prefine.h"
static const char *levelname[] = {"INFO","WARN", "ERROR", "FATAL"};
FILE *GentLog::logfd = stdout;

int GentLog::setfd(string &filename)
{
	if(filename == "") return 1;
	if((logfd = fopen(filename.c_str(), "a+")) == NULL) {
		return 0;
	}	
	return 1;
}

void GentLog::write(int levels, const char *file, const int line, const char *func, const char *format, ...)
{
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
	snprintf(str, LINEBUFSIZE, "%s %02d:%02d:%02d [%lu] %s", levelname[levels], tm->tm_hour,tm->tm_min,tm->tm_sec, pthread_self()->__sig, buf);
#else
	snprintf(str, LINEBUFSIZE, "%s %02d:%02d:%02d [%lu] %s", levelname[levels], tm->tm_hour,tm->tm_min,tm->tm_sec, pthread_self(), buf);	
#endif	
	fprintf(logfd, "%s\n", str);                                                                  	
}
