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
#include "prefine.h"
#include "gent_command.h"

class GentDb : public GentWang
{
	static GentDb *intance_;
private:
	leveldb::DB* db;
    leveldb::Options options;
public:
	GentDb();
	~GentDb();
public:
    bool Init(string &err);
    bool Put(string &key, string &value);
    bool Get(string &key,string &key);
    void del();
public:
	static GentDb *Instance();
	static void UnIntance();
};

#endif
