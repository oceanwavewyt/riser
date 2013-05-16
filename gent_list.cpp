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
}

GentList::~GentList(){
}

void GentList::Save(){
    
}

void GentList::Load(){

}