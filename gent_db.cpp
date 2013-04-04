//
//  gent_db.cpp
//  riser
//
//  Created by wyt on 13-3-24.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#include "gent_db.h"

GentDb *GentDb::intance_ = NULL;

GentDb *GentDb::Instance() {
	if(intance_ == NULL) {
		intance_ = new GentDb();
	}
	return intance_;
}

void GentDb::UnIntance() {
	if(intance_) delete intance_;
}

GentDb::GentDb()
{
    
    
}

GentDb::~GentDb()
{
    if(db) delete db;
}

bool GentDb::Get(string &key, string &value)
{
    leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value);
    return s.ok();
}

bool GentDb::Put(string &key, string &value)
{
    leveldb::Slice s2 = value;
    leveldb::Status s = db->Put(leveldb::WriteOptions(), key, s2);
    return s.ok();
}

bool GentDb::Init(string &err)
{
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);
    if(status.ok())
    {
        return true;
    }
    err = status.ToString();
    return false;
}