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

GentLink::GentLink():head(NULL) {
    HeadFind();
	string pagefile = "queue.dat";
	if(access(pagefile.c_str(),0) == -1) {                            
	    if ((fd = open(pagefile.c_str(), O_RDWR|O_CREAT,00777)) < 0){
       		cout << "open queue.dat error." << endl;                    
       		exit(-1);                                                  
    	}                                                             
	}else{
		if ((fd = open(pagefile.c_str(), O_RDWR, 00777)) < 0) {
   			 cout << "open queue.dat error." << endl;              
    		exit(-1);                                            
		}
        lseek(fd,0,SEEK_SET);
	}

}

GentLink::~GentLink(){
   munmap(head, sizeof(pageinfo));
   munmap(base, head->pagesize);
   close(fd);
   close(hfd);                                             
}

void GentLink::CreatePage() {
	uint64_t start = head->pagesize*head->page;
	if(ftruncate(fd, start + head->pagesize) < 0) {
		cout << "create page failed1." << endl;
		exit(-1);
	}
	head->page++;	
	void *ptr = mmap(NULL, head->pagesize, PROT_READ|PROT_WRITE,MAP_SHARED, fd, start);	
	if (ptr == MAP_FAILED) {
		cout << "create page failed2." << endl;
		exit(-1);                 
	}                                    
	base = reinterpret_cast<char*>(ptr);
	phead = reinterpret_cast<pagehead *>(base);
	dest = base + pageHeadLen*sizeof(pagehead);
	//init current page offset
	head->offset = 0;
	//offsetsize = pageHeadLen*sizeof(pagehead);
    //offsetsize = 0;
}

void GentLink::Init() {
	if(!head->page) {
		CreatePage();
	}else{
		uint32_t size = (head->page-1)*head->pagesize;
		cout << "size: " << size <<endl;
		base = (char *)mmap(NULL, head->pagesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		if (base == MAP_FAILED) {
    		cout << "create page failed3." << endl;                                            
    		exit(-1);                                                                          
		}
        //base = reinterpret_cast<char *>(ptr);
		phead = reinterpret_cast<pagehead *>(base);
        //exit(1);

		dest = base + phead[head->offset-1].start+phead[head->offset-1].len;
        //foreach read
        for(uint16_t i=0; i<head->offset;i++){
            string nr="";
            ReadItem(i,nr);
            cout << "i: "<<i<< "\tval: "<< nr << endl;
        }
        
	}
   	string name;
	Createid("abc", name);	
	cout << "name: " << name <<"\n" << name.size() << endl;
    

    phead[head->offset].len = name.size();
    phead[head->offset].start = dest - base;

	head->offset++;

	memcpy(dest, name.c_str(), name.size());	
	dest += name.size();
}


void GentLink::Createid(const string &quekey, string &id) {
	struct timeval tv;
	gettimeofday(&tv,NULL);
    long t = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	char buf[100];
	sprintf(buf,"%s_%ld", quekey.c_str(), t);
	id = buf;
}

void GentLink::ReadItem(uint16_t id, string &str)
{
	//if(id > pageHeadLen) return;
    assert(id<pageHeadLen);
    string ret(base+phead[id].start,phead[id].len);
	str = ret;
}

void GentLink::HeadFind()
{
    string filename = "head.dat";
	if(access(filename.c_str(),0) != -1) {
        if ((hfd = open(filename.c_str(), O_RDWR, 00777)) < 0) {
            cout << "open head.dat error." << endl;
            exit(-1);
        }
        void *h = mmap(NULL, sizeof(pageinfo), PROT_READ|PROT_WRITE,MAP_SHARED, hfd, 0);
        head = reinterpret_cast<pageinfo*>(h);
	}else {
        if ((hfd = open(filename.c_str(), O_RDWR|O_CREAT,00777)) < 0){
            cout << "open head.dat error." << endl;
            exit(-1);
		}
		if(ftruncate(hfd,sizeof(pageinfo)) < 0)
		{
			cout << "ftruncate head.dat file failed." << endl;
			exit(-1);
		}
        void *h = mmap(NULL, sizeof(pageinfo), PROT_READ|PROT_WRITE,MAP_SHARED, hfd, 0);
        head = reinterpret_cast<pageinfo*>(h);
        head->pagesize = ((uint64_t)1<<(20));
        head->offset = 0;
        head->page = 0;
    }
}
