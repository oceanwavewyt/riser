//
//  gent_list.cpp
//  bloom filter for riser
//  This soft wrap a serice for leveldb store, and support multi-threading    
//  client access it by the memcache extension of php, and telnet. Currently,
//  only support put, get and del operation. additionally, it is support bloom
//  filter algorithm for key.
//
//  Created by wyt on 13-5-6.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#include "gent_list.h"
#include "gent_config.h"
#include "gent_frame.h"
#include "gent_app_mgr.h"
//35996272   1344
//2441328   1332
GentList *GentList::intance_ = NULL;


HashInter::HashInter()
{
	uint8_t a[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
	for(int i=0; i<8; i++) {
			posval[i] = a[i];
	}
}
HashInter:: ~HashInter()
{
        close(fd);
}

uint8_t HashInter::Position(char *key,bool isget) 
{
	uint64_t h = Hash(key);
	uint64_t pos = h/8;
	int off = h%8;
	if(isget) {
		return tables[pos] & posval[off];
	}
	return tables[pos] = tables[pos] | posval[off];
}

void HashInter::Set(char *key) 
{
    AutoLock lock(&hash_lock);
	Position(key,false);
    if(successor) {
    	successor->Set(key);
    }
    msync((void*)tables,hashsize(posnum)*sizeof(byte),MS_ASYNC);
}

int HashInter::Get(char *key, int parent) 
{
	if(parent == -1) {
		parent = Position(key, true);
	}else{
		int a = Position(key, true);
		parent = parent && a;
	}
	if(successor) {
		return successor->Get(key, parent);
	}
	return parent;
}
void HashInter::Init() 
{
	if(access(filename.c_str(), 0) == -1) {
		tables = (byte *)malloc(hashsize(posnum)*sizeof(byte));
		memset(tables, 0, hashsize(posnum)*sizeof(byte));

		if ((fd = open(filename.c_str(), O_RDWR|O_CREAT,00777)) < 0){
			LOG(GentLog::ERROR, "%s open fail.", filename.c_str());
			free(tables);
			exit(-1);
		}
		write(fd,tables, hashsize(posnum)*sizeof(byte));
		close(fd);
		free(tables);
	}

	if ((fd = open(filename.c_str(), O_RDWR)) < 0){
		LOG(GentLog::ERROR, "%s open fail.", filename.c_str());
		exit(-1);
	}

	struct stat sb; 
	if ((fstat(fd, &sb)) == -1) {
		LOG(GentLog::ERROR, "%s open fail.", filename.c_str());
		exit(-1);
	}

	if((tables = (byte *)mmap(NULL, sb.st_size, PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0))
			== (void *)-1){
		LOG(GentLog::ERROR, "mmap fail.");
		exit(-1);
	}
        
}

void HashInter::SetSuccessor(HashInter *s)
{
	successor = s;
}

GentList *GentList::Instance() {
	if(intance_ == NULL) {
		intance_ = new GentList();
	}
	return intance_;
}

GentList::GentList(){
    cout << "gentlist init " <<endl;
}

GentList::~GentList(){
	list<HashInter *>::iterator it;
	for(it=hashList.begin();it!=hashList.end();it++){
    	delete (*it);                               
	}                                                
}

void GentList::Init() {
   	string path = GentFrame::Instance()->config["leveldb_db_path"];
    HashInter *h1 = new SDBMHash(path);
    HashInter *h2 = new RSHash(path);
    HashInter *h3 = new JSHash(path);
    h1->SetSuccessor(h2);
    h2->SetSuccessor(h3);
    hashList.push_back(h1);
    hashList.push_back(h2);
    hashList.push_back(h3);
    list<HashInter *>::iterator it;
    for(it=hashList.begin();it!=hashList.end();it++){
        (*it)->Init();
    }
    mainHash = h1;
}

void GentList::Save(string &key){
    char *key2 = const_cast<char *>(key.c_str());
    mainHash->Set(key2);
}

int GentList::Load(string &key){
    char *key2 = const_cast<char *>(key.c_str());
    int ret = mainHash->Get(key2, -1);
	if(ret){
    	LOG(GentLog::INFO, "bloom for key %s true.\n", key.c_str());
    }else{
		LOG(GentLog::INFO, "bloom for key %s false.\n", key.c_str());
    }
	return ret;
}
