//
//  gent_db.h
//  riser
//
//  Created by wyt on 13-3-24.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#ifndef riser_gent_db_h
#define riser_gent_db_h
#include <leveldb/db.h>
#include <leveldb/cache.h>
#include <leveldb/filter_policy.h>
#include "prefine.h"
#include "gent_command.h"
#include "gent_redis.h"

const uint64_t MAX_WRITE_CLEAR = 50000; 
struct metaData
{
	int datatype;
	uint64_t expire;
	uint64_t store_time; 
};

class GentDb
{
	static GentDb *intance_;
private:
	leveldb::DB* db;
    leveldb::Options options;
	string pathname;
	string meta_path;
	uint64_t key_num;
	uint64_t write_num;
	CommLock key_num_lock;	
	CommLock clear_lock;
	leveldb::DB* meta_db;
	bool is_clear;
public:
	GentDb();
	~GentDb();
public:
    bool Init(string &err);
    bool Put(string &key, string &value, uint64_t expire, int datatype=GentRedis::TY_STRING);
    bool Put(string &key, const char *val, uint64_t len, uint64_t expire, int datatype=GentRedis::TY_STRING);
    bool Get(string &key,string &value);
	bool Get(string &key,string &value, uint64_t &expire);
	bool Ttl(string &key, uint64_t &expire);
    bool Del(string &key);
	uint64_t Count(const string &pre="");
	uint64_t Keys(vector<string> &outvec, const string &pre="*");
	uint64_t ClearExpireKey();	
	uint64_t TotalSize();
	string &GetPath();
private:
	bool GetPathname(string &);
	void MetaSerialize(string &outstr, int datatype, uint64_t expire=0);
	void MetaUnserialize(string &str, struct metaData *dat);
public:
	static GentDb *Instance();
	static void UnIntance();
};

#endif
