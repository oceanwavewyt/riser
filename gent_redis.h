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
#include <limits.h>

static const string REDIS_INFO="+OK";
static const string REDIS_ERROR="-ERR";
static const string REDIS_AUTH_REQ="NOAUTH Authentication required.";
static const string REDIS_AUTH_FAIL="invalid password";
static const uint64_t REDIS_EXPIRE_TIME = LONG_MAX;
static const int AUTH_REQ_FAIL = -4;
static const int AUTH_FAIL = -5;
static const int MGET_REQ = -6;
class GentRedis;
class GentSubCommand
{
protected:
	uint64_t GetLength(string &str){
		return atoi(str.substr(1).c_str());	
	};
	bool IsAuth(GentRedis *r);
public:
	GentSubCommand(){};
	virtual ~GentSubCommand(){};
public:
	virtual int Parser(int,vector<string> &,const string &, GentRedis *)=0;
	virtual void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)=0;	
	virtual GentSubCommand *Clone()=0;
	virtual int ContinueParser(const string &data, GentRedis *);
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
class GentProcessMset : public GentSubCommand
{
public:
	GentProcessMset(){};
	~GentProcessMset(){};
public:
	int Parser(int,vector<string> &,const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	int ContinueParser(const string &data, GentRedis *);
	GentSubCommand *Clone()
	{
		return new GentProcessMset();
	};
private:
	int ItemParser(string &data, GentRedis *redis);
};
class GentProcessSetex : public GentSubCommand
{
public:
	GentProcessSetex(){};
	~GentProcessSetex(){};
public:
	int Parser(int,vector<string> &,const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessSetex();
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
class GentProcessTtl : public GentSubCommand
{
public:
	GentProcessTtl(){};
	~GentProcessTtl(){};
public:
	int Parser(int,vector<string> &,const string &,GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessTtl();
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

class GentProcessRandomkey : public GentSubCommand
{
public:
	GentProcessRandomkey(){};
	~GentProcessRandomkey(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessRandomkey();
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
	int TotalTableFiles(string &);
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessInfo();
	};
};
class GentProcessRep : public GentSubCommand
{
public:
	GentProcessRep(){};
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

class GentProcessAuth : public GentSubCommand
{
public:
	GentProcessAuth(){};
	~GentProcessAuth(){};
public:
	int Parser(int,vector<string> &,const string &data, GentRedis *);
	void Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis);
	GentSubCommand *Clone()
	{
		return new GentProcessAuth();
	};
};

typedef struct mset_data {
	map<string, string> keys_values;
	string content;
	string cur_key;
	size_t cur_key_end;
	size_t cur_content_len;
	size_t cur_content_start;
	int totalSetNum;
	int setNum;
	void init() {
		content = "";
		cur_key = "";
		cur_content_len = 0;
		cur_content_start = 0;
		totalSetNum = 0;
		setNum = 0;
	};
	void set() {
		setNum--;
		cur_key = "";
		cur_key_end = 0;
		cur_content_start = 0;
		cur_content_len = 0;
		content = "";
	};	
}mset_data;

class GentRedis: public GentCommand
{
	friend class GentProcessSet;
	friend class GentProcessMset;
	friend class GentProcessSetex;
	friend class GentProcessGet;
	friend class GentProcessMget;
	friend class GentProcessDel;
	friend class GentProcessTtl;
	friend class GentProcessKeys;
	friend class GentProcessExists;
	friend class GentProcessQuit;
	friend class GentProcessPing;
	friend class GentProcessInfo;
	friend class GentProcessRep;
	friend class GentProcessReply;
	friend class GentProcessSlave;
	friend class GentProcessAuth;
	friend class GentProcessRandomkey;
	static std::map<string, GentSubCommand*> commands;
public:
	enum datatype
    {
        TY_STRING = 1,
        TY_HASH = 2,
        TY_ZSET = 3,
    };
private:
	int master_auth;
	string keystr;
	mset_data mdata;
	vector<string> keyvec;
	string content;
    string repmsg;
	GentSubCommand *subc; 
	uint64_t  rlbytes;
	uint64_t  expire;
public:
    GentRedis(GentConnect *c=NULL);
    ~GentRedis();
	static void SetCommands();
public:
   int Process(const char *rbuf, uint64_t size, string &outstr);
   int ContinueProcess(const char *cbuf, uint64_t size, string &outstr);
   void Complete(string &outstr, const char *, uint64_t);
   GentCommand *Clone(GentConnect *);
   int GetStatus();
   bool Init(string &msg);
   bool Slave();
   int GetAuth(){return auth;};
private:
	int Split(const string &str, const string &delimit, vector<string> &v);
	uint64_t GetLength(string &str);
	int ParseCommand(const string &str);
	void Info(const string &msg, string &outstr, const string &);
    void ProcessStats(string &);
};
#endif
