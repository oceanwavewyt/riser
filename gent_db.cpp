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

bool GentDb::Del(string &key)
{
	leveldb::Status s = db->Delete(leveldb::WriteOptions(), key);
	return s.ok();
}

uint64_t GentDb::TotalSize() {
	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	leveldb::Range ranges[1];
	it->SeekToFirst();
	if(!it->Valid()) {
		delete it;
		return 0;
	}
	std::string first = it->key().ToString();
	it->SeekToLast();
	if(!it->Valid()) {
		delete it;
		return 0;
	}
	std::string end = it->key().ToString();
	ranges[0] = leveldb::Range(first, end);
	uint64_t sizes[1];
	db->GetApproximateSizes(ranges, 1, sizes);
	delete it;
	return sizes[0];
}

uint64_t GentDb::Count(const string &pre) 
{
	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	uint64_t num=0;
	bool first = false;
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		if(pre != "") {
			string curkey(it->key().ToString());
			if(curkey.substr(0, pre.size())!=pre){
				if(first) break; 
				continue;
			}
			first = true;
			num++;
		}else{
			num++;
		}
	}
	delete it;
	return num;
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
	
    options.write_buffer_size = WRITE_BUFFER_SIZE*1024*1024;
    options.target_file_size = TARGET_FILE_SIZE;
    if(config["leveldb_target_file_size_mb"] != "") {
        int size = atoi(config["leveldb_target_file_size_mb"].c_str());
        if(size > 2) {
            options.target_file_size = size;
        }
    }
	leveldb::Status status = leveldb::DB::Open(options, pathname , &db);
    if(status.ok())
    {
        return true;
    }
    err = status.ToString();
    return false;
}
