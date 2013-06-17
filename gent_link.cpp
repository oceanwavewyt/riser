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
	string filename = "head.dat";
	if(access(filename.c_str(),0) == -1) {
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
        
		//close(hfd);
	}
    if(!head) {
        if ((hfd = open(filename.c_str(), O_RDWR, 00777)) < 0) {
            cout << "open head.dat error." << endl;
            exit(-1);
        }
        void *h = mmap(NULL, sizeof(pageinfo), PROT_READ|PROT_WRITE,MAP_SHARED, hfd, 0);
        head = reinterpret_cast<pageinfo*>(h);
    }
	//msync((void*)head,sizeof(pageinfo),MS_ASYNC);
	cout << "pagesize: " << head->pagesize << endl;
	cout << "page: " << head->page << endl;
	cout << "start offset: " << head->offset << endl;
    
	string pagefile = "queue.dat";
	if(access(pagefile.c_str(),0) == -1) {                            
	    if ((fd = open(pagefile.c_str(), O_RDWR|O_CREAT,00777)) < 0){
       		cout << "open queue.dat error." << endl;                    
       		exit(-1);                                                  
    	}                                                             
	}else{
		if ((fd = open(filename.c_str(), O_RDWR, 00777)) < 0) { 
   			 cout << "open queue.dat error." << endl;              
    		exit(-1);                                            
		}                                                        
	}

}

GentLink::~GentLink(){
   //munmap(head, sizeof(pageinfo));
   //munmap(base, head->pagesize);
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
	//phead = reinterpret_cast<pagehead *>(base);
	//dest = base + pageHeadLen*sizeof(pagehead);
    dest = base;
	//init current page offset
	head->offset = 0;
	//offsetsize = pageHeadLen*sizeof(pagehead);
    offsetsize = 0;
}

void GentLink::Init() {
	if(!head->page) {
		CreatePage();
	}else{
		uint32_t size = (head->page-1)*head->pagesize;
		cout << "size: " << size <<endl;
		void *ptr = mmap(NULL, head->pagesize, PROT_READ|PROT_WRITE,MAP_SHARED, fd, size);
		if (ptr == MAP_FAILED) {                                                               
    		cout << "create page failed3." << endl;                                            
    		exit(-1);                                                                          
		}                                                                                      
		base = reinterpret_cast<char *>(ptr);
		//phead = reinterpret_cast<pagehead *>(base);
		string nr = "";
		ReadItem(head->offset-1, nr);
        cout << "curitem: "<< nr << endl;
        exit(1);
		//dest = base + phead[head->offset].pos + nr.size();
		dest = base + head->offset;                                                                           
	}
   	string name;
	Createid("abc", name);	
	cout << "name: " << name <<"\n" << name.size() << endl;
    //exit(1);
	//struct item it;
	head->offset++;
	//it.id = head->offset;
	//it.len = name.size();
	//phead[head->offset].pos = dest-base;
	//char *its = reinterpret_cast<char*>(&it);
	//memcpy(dest, its, sizeof(struct item));
	//dest += sizeof(struct item);
	memcpy(dest, name.c_str(), name.size());	
	dest += name.size();
	
	//string path = GentFrame::Instance()->config["leveldb_db_path"];
	cout << "offset: " << head->offset << endl;
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
	cout <<"ReadItem: "<<id << " base: " << base <<endl;
	//char *t = base + phead[id].pos;
	//item *it = reinterpret_cast<item *>(t);
	//cout << "item: " << it->len << endl;
	//char *s = t + sizeof(item);
	//string ret(s, it->len);
    string ret(base + (id * 17),17);
	str = ret;	
}
