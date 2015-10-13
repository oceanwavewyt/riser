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
	virtual ~GentSubCommand(){};
public:
	virtual int Parser(int,vector<string> &,const string &, GentRedis *)=0;
	virtual void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)=0;	
	virtual GentSubCommand *Clone()=0;
};
class GentProcessGet : public GentSubCommand
{
public:
	GentProcessGet(){};
	~GentProcessGet(){};
public:
	int Parser(int,vector<string> &, const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessGet();
	};
};
class GentProcessSet : public GentSubCommand
{
public:
	GentProcessSet(){};
	~GentProcessSet(){};
public:
	int Parser(int,vector<string> &,const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessSet();
	};
};
class GentProcessMget : public GentSubCommand
{
public:
	GentProcessMget(){};
	~GentProcessMget(){};
public:
	int Parser(int,vector<string> &,const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessMget();
	};
};
class GentProcessDel : public GentSubCommand
{
public:
	GentProcessDel(){};
	~GentProcessDel(){};
public:
	int Parser(int,vector<string> &,const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessDel();
	};
};
class GentProcessQuit : public GentSubCommand
{
public:
	GentProcessQuit(){};
	~GentProcessQuit(){};
public:
	int Parser(int,vector<string> &,const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessQuit();
	};
};

class GentProcessKeys : public GentSubCommand
{
public:
	GentProcessKeys(){};
	~GentProcessKeys(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessKeys();
	};
};
class GentProcessExists : public GentSubCommand
{
public:
	GentProcessExists(){};
	~GentProcessExists(){};
public:
	int Parser(int,vector<string> &,const string &data,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessExists();
	};
};
class GentProcessPing : public GentSubCommand
{
public:
	GentProcessPing(){};
	~GentProcessPing(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessPing();
	};
};
class GentProcessInfo : public GentSubCommand
{
public:
	GentProcessInfo(){};
	~GentProcessInfo(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessInfo();
	};
};
class GentProcessRep : public GentSubCommand
{
	string msg;
public:
	GentProcessRep(){msg="";};
	~GentProcessRep(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessRep();
	};
};
class GentProcessReply : public GentSubCommand
{
public:
	GentProcessReply(){};
	~GentProcessReply(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessReply();
	};
};
class GentProcessSlave : public GentSubCommand
{
public:
	GentProcessSlave(){};
	~GentProcessSlave(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessSlave();
	};
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
	friend class GentProcessReply;
	friend class GentProcessSlave;
	static std::map<string, GentSubCommand*> commands;
private:
	string auth;	
	string keystr;
	vector<string> keyvec;
	string content;
	GentSubCommand *subc; 
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
   bool Slave();
private:
	int Split(const string &str, const string &delimit, vector<string> &v);
	uint64_t GetLength(string &str);
	int ParseCommand(const string &str);
	string Info(const string &msg, const string &);
    void ProcessStats(string &);
};
#endif
