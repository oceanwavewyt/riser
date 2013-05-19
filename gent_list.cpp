//
//  gent_list.cpp
//  riser
//
//  Created by wyt on 13-5-6.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#include "gent_list.h"
//35996272   1344
//2441328   1332
GentList *GentList::intance_ = NULL;

GentList *GentList::Instance() {
	if(intance_ == NULL) {
		intance_ = new GentList();
	}
	return intance_;
}

GentList::GentList(){
    cout << "haha gentlist init " <<endl;
}

GentList::~GentList(){
}

void GentList::Init() {
    HashInter *h1 = new SDBMHash();
    HashInter *h2 = new RSHash();
    HashInter *h3 = new JSHash();
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

void GentList::Load(string &key){
    char *key2 = const_cast<char *>(key.c_str());
    if(mainHash->Get(key2,-1)){
    cout <<"bloom load key: true....." << endl;
    }else{
    cout <<"bloom load key: false....." << endl;
    }
}