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
#include "gent_thread.h"

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

GentDb::GentDb():key_num(0)
{
	meta_path = "/meta";    
	is_clear = false;
	write_num = 0;
}

GentDb::~GentDb()
{
    if(db) delete db;
	if(options.block_cache) delete options.block_cache;
}

bool GentDb::Get(string &key, string &value)
{
	string metaData;
	leveldb::Status s= meta_db->Get(leveldb::ReadOptions(), key, &metaData);
	if(s.ok()) {
		struct metaData dat;
		MetaUnserialize(metaData, &dat);	
		if(dat.expire != 0 && dat.expire < (unsigned long long)time(NULL)) {
			value = "";
			return false;
		}
	}    
	s = db->Get(leveldb::ReadOptions(), key, &value);
    return s.ok();
}

bool GentDb::Get(string &key, string &value, uint64_t &expire)
{
	string metaData;
	leveldb::Status s= meta_db->Get(leveldb::ReadOptions(), key, &metaData);
	if(s.ok()) {
		struct metaData dat;
		GentDb::MetaUnserialize(metaData, &dat);
		expire = dat.expire;
	}else{
		expire = 0;
	}    
	s = db->Get(leveldb::ReadOptions(), key, &value);
    return s.ok();
}

bool GentDb::Put(string &key, string &value,uint64_t expire, int datatype)
{
	AutoLock lockclear(&clear_lock);	
    leveldb::Slice s2 = value;
    leveldb::Status s = db->Put(leveldb::WriteOptions(), key, s2);
    if(key_num > 0) {
		AutoLock lock(&key_num_lock);
		key_num++;
	}
	if(s.ok()) {
		string meData;
		MetaSerialize(meData, datatype, expire);
		leveldb::Slice s3 = meData;
		meta_db->Put(leveldb::WriteOptions(), key, s3);
	}
	write_num++;
	return s.ok();
}

bool GentDb::Put(string &key, const char *val, uint64_t len, uint64_t expire, int datatype)
{
	AutoLock lockclear(&clear_lock);
	if(write_num > MAX_WRITE_CLEAR && is_clear == false) {
		GentThread::Intance()->StartClear();
		write_num = 0;	
	}
    leveldb::Status s = db->Put(leveldb::WriteOptions(), key, leveldb::Slice(val,len));
    if(key_num > 0) {
		AutoLock lock(&key_num_lock);
		key_num++;
	}
	if(s.ok()) {
		string meData;
		MetaSerialize(meData, datatype, expire);
		leveldb::Slice s3 = meData;
		meta_db->Put(leveldb::WriteOptions(), key, s3);
	}
	write_num++;
	return s.ok();
}

bool GentDb::Del(string &key)
{
	leveldb::Status s = db->Delete(leveldb::WriteOptions(), key);
	if(key_num > 0) {
		AutoLock lock(&key_num_lock);
		key_num--;
	}
	if(s.ok()) {
		meta_db->Delete(leveldb::WriteOptions(), key);
	}
	return s.ok();
}

bool GentDb::Ttl(string &key, uint64_t &expire)
{
	string metaData;
	leveldb::Status s= meta_db->Get(leveldb::ReadOptions(), key, &metaData);
	if(s.ok()) {
		struct metaData dat;
		MetaUnserialize(metaData, &dat);
		expire = dat.expire;
		return true;
	}else{
		expire = 0;
	}
	string value;    
	s = db->Get(leveldb::ReadOptions(), key, &value);
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
	if(pre == "" && key_num > 0) return key_num;
	leveldb::ReadOptions options;
	options.snapshot = meta_db->GetSnapshot();
	leveldb::Iterator* it = meta_db->NewIterator(options);
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
	meta_db->ReleaseSnapshot(options.snapshot);
	delete it;
	if(pre == "" && key_num == 0) {
		AutoLock lock(&key_num_lock);
		key_num = num;
	}
	return num;
}

uint64_t GentDb::ClearExpireKey()
{
	AutoLock lockclear(&clear_lock);
	is_clear = true;	
	leveldb::ReadOptions options;
	options.snapshot = meta_db->GetSnapshot();
	leveldb::Iterator* it = meta_db->NewIterator(options);
	uint64_t num=0;
	for (it->SeekToFirst(); it->Valid(); it->Next()) {
		struct metaData dat;
		string metaData(it->value().ToString());
		MetaUnserialize(metaData, &dat);
		if(dat.expire == 0) continue;
		if(dat.expire > (unsigned long long)time(NULL)) continue;
		string curkey(it->key().ToString());
		Del(curkey);		
		num++;
	}
	is_clear = false;
	meta_db->ReleaseSnapshot(options.snapshot);
	delete it;
	return num;
}

uint64_t GentDb::Keys(vector<string> &outvec, const string &pre) 
{
	leveldb::ReadOptions options;
	options.snapshot = meta_db->GetSnapshot();
	leveldb::Iterator* it = meta_db->NewIterator(options);
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
			num++;
			first = false;
		}else{
			outvec.push_back(it->key().ToString());
			num++;
		}
	}
	meta_db->ReleaseSnapshot(options.snapshot);
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
	string sep = config["leveldb_db_path"].substr(config["leveldb_db_path"].size()-1,1);
	if(sep != "/") {
		config["leveldb_db_path"] = config["leveldb_db_path"]+"/";
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
	if(config["leveldb_write_buffer_size_mb"] != "") {
        int size = atoi(config["leveldb_write_buffer_size_mb"].c_str());
        if(size > 2) {
            options.write_buffer_size = size*1024*1024;
        }
    }	
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
		if(size > 1) {
			options.block_size = size * 1024;
		} 
	}
	
	if(config["leveldb_compression"] != "" && config["leveldb_compression"] != "true") {
		options.compression = leveldb::kNoCompression;
	}

	options.filter_policy = leveldb::NewBloomFilterPolicy(10);
	leveldb::Status status = leveldb::DB::Open(options, pathname , &db);
    if(status.ok())
    {
		leveldb::Options op;
		op.create_if_missing = true;
		op.target_file_size = TARGET_FILE_SIZE;
		if(config["leveldb_target_file_size_mb"] != "") {
        	int size = atoi(config["leveldb_target_file_size_mb"].c_str());
        	if(size > 2) {
            	op.target_file_size = size;
        	}
    	}
		op.write_buffer_size = 100*1024*1024;
		string mp = pathname+meta_path;
		leveldb::Status meta_status = leveldb::DB::Open(op, mp , &meta_db);
        if(meta_status.ok()) return true;
		return false;
    }
    err = status.ToString();
    return false;
}

string &GentDb::GetPath() {
	return pathname; 
}

void GentDb::MetaSerialize(string &outstr, int datatype, uint64_t expire) {
	char buf[100]={0};
	if(expire == 0 || expire > REDIS_EXPIRE_TIME) {
		snprintf(buf, 100, "%d*0000000000%lu",datatype,  time(NULL));
	}else{
		snprintf(buf, 100, "%d+%llu%lu",datatype, (unsigned long long)expire, time(NULL));
	}
	outstr = buf;
}

void GentDb::MetaUnserialize(string &str, struct metaData *dat) {
	dat->datatype = atoi(str.substr(0,1).c_str());
	if(str.substr(1,1) == "*") {
		dat->expire = 0;	
	}else{
		dat->expire = strtoull(str.substr(2,10).c_str(),NULL,10);
	}
	dat->store_time = strtoull(str.substr(12,10).c_str(), NULL,10);
}
