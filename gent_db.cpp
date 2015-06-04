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
	if(options.block_cache) delete options.block_cache;
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

uint64_t GentDb::Keys(vector<string> &outvec, const string &pre) 
{
	leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
	uint64_t num=0;
	bool first = true;
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		if(pre != "*") {
			string curkey(it->key().ToString());
			if(curkey.substr(0, pre.size())!=pre) {
				if(first == false) {
					break;
				}else{
					continue;
				}
			}
			outvec.push_back(curkey);
			cout << "item: "<<curkey <<endl; 
			num++;
			first = false;
		}else{
			outvec.push_back(it->key().ToString());
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
	if(config["leveldb_block_cache_size_mb"] != "") {
		int size = atoi(config["leveldb_block_cache_size_mb"].c_str());
		if(size > 8) {
			options.block_cache = leveldb::NewLRUCache(size * 1048576);
		} 
	}

	if(config["leveldb_block_size_kb"] != "") {
		int size = atoi(config["leveldb_block_size_kb"].c_str());
		if(size < 1) {
			options.block_size = size * 1024;
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
