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

#include "gent_link.h"
#include "gent_config.h"
#include "gent_frame.h"
#include "gent_app_mgr.h"

GentLink *GentLink::intance_ = NULL;

GentLink *GentLink::Instance() {
	if(intance_ == NULL) {
		intance_ = new GentLink();
	}
	return intance_;
}

GentLink::GentLink(){
}

GentLink::~GentLink(){
                                                
}

void GentLink::Init() {
   	string id;
	Createid("abc", id);	
	cout << "id: " << id << endl;
	//string path = GentFrame::Instance()->config["leveldb_db_path"];
}

void GentLink::Createid(const string &quekey, string &id) {
	struct timeval tv;
	gettimeofday(&tv,NULL);
    long t = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	char buf[100];
	sprintf(buf,"%s_%ld", quekey.c_str(), t);
	id = buf;
}

