//
//  gent_list.h
//  riser
//
//  Created by wyt on 13-5-4.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#ifndef riser_gent_list_h
#define riser_gent_list_h
#include "prefine.h"
#include <sys/mman.h> 
#include <sys/stat.h> 

typedef struct pageinfo
{
	uint16_t page;
	//write offset the inner of page 
	uint64_t offset;
	uint64_t pagesize;
	uint16_t curpage;
	uint16_t curid; 
}pageinfo;

typedef struct item
{
	uint64_t id;
	uint8_t len;	
}item;

class GentLink
{
    static GentLink *intance_;
	pageinfo *pagehead;
	int fd;
	int hfd;
	char *base;
	char *dest;	
public:
	static GentLink *Instance();
	static void UnInstance();
public:
    GentLink();
    ~GentLink();
public:
    void Init();
private:
	void CreatePage();
	void Createid(const string &quekey,string &);
};

#endif
