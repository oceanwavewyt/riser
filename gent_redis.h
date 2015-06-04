//
//  gent_redis.h
//  riser for redis command
//
//  Created by wyt on 13-1-27.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#ifndef riser_gent_redis_h
#define riser_gent_redis_h

#include "gent_command.h"
#include "gent_connect.h"
static const string REDIS_INFO="+OK";
static const string REDIS_ERROR="-ERR";

class GentRedis: public GentCommand
{
	string keystr;
	vector<string> keyvec;
	string content;
	uint8_t commandtype;
	uint64_t  rlbytes;
public:
    GentRedis(GentConnect *c=NULL);
    ~GentRedis();
public:
   int Process(const char *rbuf, uint64_t size, string &outstr);	
   void Complete(string &outstr, const char *, uint64_t);
   GentCommand *Clone(GentConnect *);
   int GetStatus();
   bool Init(string &msg);
private:
	int Split(const string &str, const string &delimit, vector<string> &v);
	uint64_t GetLength(string &str);
	int ParseCommand(const string &str);
	string Info(const string &msg, const string &);
    void ProcessGet(string &);
    void ProcessSet(string &outstr, const char *recont, uint64_t len);
	void ProcessMultiGet(string &);
    void ProcessDel(string &); 
    void ProcessStats(string &);
    void ProcessKeys(string &);
	void ProcessExists(string &outstr);
};
#endif
