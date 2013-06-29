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

GentLinkMgr *GentLinkMgr::intance_ = NULL;

GentLinkMgr *GentLinkMgr::Instance() {
	if(intance_ == NULL) {
		intance_ = new GentLinkMgr();
	}
	return intance_;
}

GentLinkMgr::GentLinkMgr(){
}
GentLinkMgr::~GentLinkMgr(){
}

GentLink *GentLinkMgr::GetLink(const string &queueName)
{
	map<string,GentLink*>::iterator it = links.find(queueName);	
	if(it == links.end()) return NULL;
	return links[queueName];	
}

void GentLinkMgr::Init() {
	string name = "abc";
	links[name] = new GentLink(name);	
	links[name]->Init();
}


GentLink::GentLink(const string &n):head(NULL),name(n) {
    HeadFind();
	//string pagefile = "queue.dat";
	//fd = OpenFile(pagefile);
}

GentLink::~GentLink(){
   munmap(head, sizeof(pageinfo));
   munmap(base, head->pagesize);
   close(fd);
   close(hfd);                                             
}

void GentLink::CreatePage() {
	string pageFile;
	uint16_t pid = head->page + 1;
	GetPageFile(pid, pageFile);
	fd = OpenFile(pageFile);
	if(ftruncate(fd, head->pagesize) < 0) {
		LOG(GentLog::ERROR, "ftruncate page failed.");
		exit(-1);                                  
	}
	head->act[head->page].pageid = pid;
	head->act[head->page].active = 0; 
	head->page++;	
	void *ptr = mmap(NULL, head->pagesize, PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0);	
	if (ptr == MAP_FAILED) {
		cout << "create page failed2." << endl;
		LOG(GentLog::ERROR, "mmap page failed.");
		exit(-1);                 
	}                                    
	base = reinterpret_cast<char*>(ptr);
	phead = reinterpret_cast<pagehead *>(base);
	dest = base + PAGEHEADLEN*sizeof(pagehead);
	//init current page offset
	head->offset = 0;
	head->curpage = 1;
}

void GentLink::Init() {
	if(!head->page || head->offset >= PAGEHEADLEN) {
		CreatePage();                                           
	}else{
		string filename;
		GetPageFile(head->page, filename);
		fd = OpenFile(filename);
		base = (char *)mmap(NULL, head->pagesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		if (base == MAP_FAILED) {
    		cout << "create page failed3." << endl;
			LOG(GentLog::ERROR, "mmap page failed.");                                            
    		exit(-1);                                                                          
		}
		phead = reinterpret_cast<pagehead *>(base);

		dest = base + phead[head->offset-1].start+phead[head->offset-1].len;
        //foreach read
        /*
        for(uint16_t i=0; i<head->offset;i++) {
            string nr="";
            ReadItem(i,nr);
            cout << "i: "<<i<< "\tval: "<< nr << endl;
        }
        */
	}
}

int GentLink::Push(const string &str)
{
	if(head->offset >= PAGEHEADLEN) {
		munmap(base, head->pagesize);
		CreatePage();
	}
	string curkey;
	GenerateId(name, curkey);	
	cout << "push key: " << curkey <<"\n" << curkey.size() << endl;
	LOG(GentLog::INFO, "push item %s", curkey.c_str());
	WriteItem(curkey);
	return 1;
}

int GentLink::Pop(string &key)
{
	if(head->curpage == head->page) {
		rbase = base;
	}else{
		assert(head->curpage<=head->page);
		string filename;
		GetPageFile(head->curpage, filename);	
		int rfd = OpenFile(filename);
		rbase = (char *)mmap(NULL, head->pagesize, PROT_READ|PROT_WRITE, MAP_SHARED, rfd, 0); 
		if (rbase == MAP_FAILED) {                                                           
    		cout << "mmap read page failed." << endl;
			LOG(GentLog::ERROR, "mmap read page failed.");                                         
    		exit(-1);                                                                       
		}                                                                                   
	}	
	key = "";
	ReadItem(key);
	if(key == ""){
		LOG(GentLog::INFO, "read queue is null.");
		cout << "read queue is null" << endl;
	}else{
		LOG(GentLog::INFO, "read queue %s.", key.c_str());
		cout << "read queue: "<< key << endl;
	}
	return 1;
}

void GentLink::GenerateId(const string &quekey, string &id) {
	struct timeval tv;
	gettimeofday(&tv,NULL);
    long t = tv.tv_sec * 1000 + tv.tv_usec / 1000;
	char buf[100];
	sprintf(buf,"%s_%ld", quekey.c_str(), t);
	id = buf;
}

void GentLink::WriteItem(const string &data) {
    phead[head->offset].len = data.size();
    phead[head->offset].start = dest - base;
	head->offset++;
	memcpy(dest, data.c_str(), data.size());	
	dest += data.size();
}

bool GentLink::ReadItem(string &str)
{
	//if(id > pageHeadLen) return;
    //assert(id<PAGEHEADLEN);
	assert(head->curid<PAGEHEADLEN);
	if(head->curpage == head->page) {
		if(head->curid == head->offset) return false;
	}
    string ret(rbase+phead[head->curid].start,phead[head->curid].len);
	str = ret;
	head->curid++;
	/*in excess of PAGEHEADLEN*/
	if(head->curpage != head->page && head->curid >= PAGEHEADLEN) {
		head->curpage++;
		head->curid = 0;
	}
	return true;
}

int GentLink::OpenFile(string &filename, bool create)
{
	int f = O_RDWR;
	if(access(filename.c_str(),0) == -1) {
		assert(create);
		f = f|O_CREAT;
	}
	int fid = open(filename.c_str(), f, 00777);
	assert(fid>0);
	return fid;
}

void GentLink::HeadFind()
{
	assert(name!="");
    string filename = name+"_head.dat";
	hfd = OpenFile(filename);
	struct stat st;
	if(stat(filename.c_str(), &st) == -1) {
		cout << "stat head.dat error." << endl;
		LOG(GentLog::ERROR, "stat head.dat failed.");
		exit(-1);                              
	}
	if(!st.st_size) {
		if(ftruncate(hfd,sizeof(pageinfo)) < 0) {
			LOG(GentLog::ERROR, "ftruncate head.dat failed.");                                                     
	    	cout << "ftruncate head.dat file failed." << endl;
    		exit(-1);                                         
		}                                                     
	}
	void *h = mmap(NULL, sizeof(pageinfo), PROT_READ|PROT_WRITE,MAP_SHARED, hfd, 0); 
	head = reinterpret_cast<pageinfo*>(h);                                           
	if(! head->pagesize) {
		head->pagesize = PAGESIZE;
		head->offset = 0;
		head->page = 0;
	}
}

void GentLink::GetPageFile(uint16_t pageid, string &file)
{
	char buf[100]={0};
	snprintf(buf, 100, "%s_queue_%u.dat",name.c_str(), pageid);
	file = buf;
}
