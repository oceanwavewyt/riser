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
class GentRedis;
class GentSubCommand
{
protected:
	uint64_t GetLength(string &str){
		return atoi(str.substr(1).c_str());	
	};
public:
	GentSubCommand(){};
	~GentSubCommand(){};
public:
	virtual int Parser(int,vector<string> &,const string &, GentRedis *)=0;
	virtual void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)=0;	
};
class GentProcessGet : public GentSubCommand
{
public:
	GentProcessGet(){};
	~GentProcessGet(){};
public:
	int Parser(int,vector<string> &, const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
};
class GentProcessSet : public GentSubCommand
{
public:
	GentProcessSet(){};
	~GentProcessSet(){};
public:
	int Parser(int,vector<string> &,const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
};
class GentProcessMget : public GentSubCommand
{
public:
	GentProcessMget(){};
	~GentProcessMget(){};
public:
	int Parser(int,vector<string> &,const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
};
class GentProcessDel : public GentSubCommand
{
public:
	GentProcessDel(){};
	~GentProcessDel(){};
public:
	int Parser(int,vector<string> &,const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
};
class GentProcessQuit : public GentSubCommand
{
public:
	GentProcessQuit(){};
	~GentProcessQuit(){};
public:
	int Parser(int,vector<string> &,const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
};

class GentProcessKeys : public GentSubCommand
{
public:
	GentProcessKeys(){};
	~GentProcessKeys(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
};
class GentProcessExists : public GentSubCommand
{
public:
	GentProcessExists(){};
	~GentProcessExists(){};
public:
	int Parser(int,vector<string> &,const string &data,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
};
class GentProcessPing : public GentSubCommand
{
public:
	GentProcessPing(){};
	~GentProcessPing(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
};
class GentProcessInfo : public GentSubCommand
{
public:
	GentProcessInfo(){};
	~GentProcessInfo(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
};
class GentProcessRep : public GentSubCommand
{
public:
	GentProcessRep(){};
	~GentProcessRep(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
};

class GentRedis: public GentCommand
{
	friend class GentProcessSet;
	friend class GentProcessGet;
	friend class GentProcessMget;
	friend class GentProcessDel;
	friend class GentProcessKeys;
	friend class GentProcessExists;
	friend class GentProcessQuit;
	friend class GentProcessPing;
	friend class GentProcessInfo;
	friend class GentProcessRep;
	static std::map<string, GentSubCommand*> commands;
private:	
	string keystr;
	vector<string> keyvec;
	string content;
	GentSubCommand *c; 
	uint64_t  rlbytes;
public:
    GentRedis(GentConnect *c=NULL);
    ~GentRedis();
	static void SetCommands();
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
    void ProcessStats(string &);
};
#endif
