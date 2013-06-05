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

GentLink::GentLink():pagehead(NULL) {
	string filename = "pagehead.dat";
	if(access(filename.c_str(),0) == -1) {
		if ((hfd = open(filename.c_str(), O_RDWR|O_CREAT,00777)) < 0){
		   cout << "open pagehead.dat error." << endl;                                        
   		   exit(-1);                                                
		}
		if(ftruncate(hfd,sizeof(pageinfo)) < 0)
		{
			cout << "ftruncate page.dat file failed." << endl;
			exit(-1);
		}
		close(hfd);	                                                            
	}
	if ((hfd = open(filename.c_str(), O_RDWR, 00777)) < 0) {
		cout << "open pagehead.dat error." << endl;
		exit(-1);                              
	}	
	pagehead = (pageinfo *)mmap(NULL, sizeof(pageinfo), PROT_READ|PROT_WRITE,MAP_SHARED, hfd, 0);	
	pagehead->pagesize = ((uint64_t)1<<(20));	
	//msync((void*)pagehead,sizeof(pageinfo),MS_ASYNC);
	cout << "pagesize: " << pagehead->pagesize << endl;
	cout << "page: " << pagehead->page << endl;


	string pagefile = "page.dat";
	if(access(pagefile.c_str(),0) == -1) {                            
	    if ((fd = open(pagefile.c_str(), O_RDWR|O_CREAT,00777)) < 0){
       		cout << "open page.dat error." << endl;                    
       		exit(-1);                                                  
    	}                                                             
	}else{
		if ((fd = open(filename.c_str(), O_RDWR, 00777)) < 0) { 
   			 cout << "open page.dat error." << endl;              
    		exit(-1);                                            
		}                                                        
	}
	

}

GentLink::~GentLink(){
   close(fd);
   close(hfd);                                             
}

void GentLink::CreatePage() {
	uint64_t start = pagehead->pagesize*pagehead->page;
	if(ftruncate(fd, start + pagehead->pagesize) < 0) {
		cout << "create page failed1." << endl;
		exit(-1);
	}
	pagehead->page++;	
	void *ptr = mmap(NULL, pagehead->pagesize, PROT_READ|PROT_WRITE,MAP_SHARED, fd, start);	
	if (ptr == MAP_FAILED) {
		cout << "create page failed2." << endl;
		exit(-1);                 
	}                                    
	base = reinterpret_cast<char*>(ptr);
	dest = base;
	//init current page offset
	pagehead->offset = 0;
}

void GentLink::Init() {
	if(!pagehead->page) {
		CreatePage();
	}else{
		uint32_t size = (pagehead->page-1)*pagehead->pagesize;
		cout << "size: " << size <<endl;
		void *ptr = mmap(NULL, pagehead->pagesize, PROT_READ|PROT_WRITE,MAP_SHARED, fd, size);
		if (ptr == MAP_FAILED) {                                                               
    		cout << "create page failed3." << endl;                                            
    		exit(-1);                                                                          
		}                                                                                      
		base = reinterpret_cast<char*>(ptr);                                                   
		dest = base + pagehead->offset;                                                                           
	}
   	string name;
	Createid("abc", name);	
	cout << "name: " << name << endl;
	struct item it;
	it.id = pagehead->offset;
	it.len = name.size();
	memcpy(dest, &it, sizeof(struct item));
	dest += sizeof(struct item);
	memcpy(dest, name.c_str(), name.size());	
	dest += name.size();
	pagehead->offset += sizeof(struct item)+name.size();
	//string path = GentFrame::Instance()->config["leveldb_db_path"];
	cout << "offset: " << pagehead->offset << endl;
}


void GentLink::Createid(const string &quekey, string &id) {
	struct timeval tv;
	gettimeofday(&tv,NULL);
    long t = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	char buf[100];
	sprintf(buf,"%s_%ld", quekey.c_str(), t);
	id = buf;
}

