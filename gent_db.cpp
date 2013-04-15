//
//  gent_db.cpp
//  riser
//
//  Created by wyt on 13-3-24.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#include "gent_db.h"
#include "gent_config.h"
#include "gent_frame.h"

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

bool GentDb::GetPathname(string &err)
{
	GentConfig &config = GentFrame::Instance()->config;
	if(config["leveldb_db_path"]=="" || config["leveldb_db_name"]=="") {
		err = "db path and db name not config";
		return false;
	}
	if(access(config["leveldb_db_path"].c_str(), W_OK | R_OK) != 0) {
		err = config["leveldb_db_path"]  + "  not access";
		return false;
	}
	pathname = config["leveldb_db_path"]+config["leveldb_db_name"];
	return true;
}

bool GentDb::Init(string &err)
{
	if(!GetPathname(err)) {
		return false;
	}
    options.create_if_missing = true;
    GentConfig &config = GentFrame::Instance()->config;
	if(config["leveldb_max_open_files"] != "") {
		options.max_open_files = atoi(config["leveldb_max_open_files"].c_str());
	}	
	
    options.write_buffer_size = 8*1024*1024;
    options.target_file_size = 64;
	leveldb::Status status = leveldb::DB::Open(options, pathname , &db);
    if(status.ok())
    {
        return true;
    }
    err = status.ToString();
    return false;
}
