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

const uint16_t pageHeadLen = 12000;

typedef struct pagehead
{
    uint32_t start;
    uint16_t len;
}pagehead;

typedef struct pageinfo
{
	uint16_t page;
	//write offset id the inner of page 
	uint16_t offset;
	uint64_t pagesize;
	uint16_t curpage;
	uint16_t curid; 
}pageinfo;

typedef struct item
{
	uint16_t id;
	uint8_t len;	
}item;

class GentLink
{
    static GentLink *intance_;
	pageinfo *head;
	int fd;
	int hfd;
	char *base;
	pagehead *phead;
	char *dest;
	//page actal offset 
	uint32_t offsetsize;	
public:
	static GentLink *Instance();
	static void UnInstance();
public:
    GentLink();
    ~GentLink();
public:
    void Init();
private:
    void HeadFind();
	void CreatePage();
	void Createid(const string &quekey,string &);
	void ReadItem(uint16_t id, string &str);
};

#endif
