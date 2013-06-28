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

const uint32_t PAGESIZE = ((uint64_t)1<<(20))*64;
const uint32_t PAGEHEADLEN = 12000 * 64;

typedef struct pagehead
{
    uint32_t start;
    uint16_t len;
}pagehead;

struct pageact{
	uint16_t pageid;
	uint16_t active;
};

typedef struct pageinfo
{
	uint64_t pagesize;
	/*multi page*/
	struct pageact act[1000];
	/*current write page*/
	uint16_t page;
	/*write offset id the inner of page*/
	uint16_t offset;
	/*current read page*/
	uint16_t curpage;
	/*current read postion*/
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
	int Push(const string &str);
private:
    void HeadFind();
	int  OpenFile(string &filename);
	void CreatePage();
	void WriteItem(const string &data);
	void GenerateId(const string &quekey,string &);
	void ReadItem(uint16_t id, string &str);
	void GetPageFile(uint16_t pageid, string &file);
};

#endif
