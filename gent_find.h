//
//  gent_list.h
//  riser
//
//  Created by wyt on 13-5-4.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#ifndef riser_gent_find_h
#define riser_gent_find_h
#include "prefine.h"

typedef struct nodes{
    int base;
    int check;
    short child_count;
    short is_word;
    char name[5];
}node;

typedef struct queue_t {
    int index;
    struct queue_t *next;
} queue_t;

typedef struct {
    int length;
    queue_t *head_ex; //exist head
    queue_t *head_noex;
} nodestats_t;		

class GentFind
{
public:
	GentFind();
	~GentFind();
public:
	void Search(const string &str, std::vector<string> &v);	

};
class GentFindMgr
{
	node **nodestable;
    nodestats_t nodestats;
	int length;
	static GentFindMgr *intance_;
public:
	static GentFindMgr *Instance();
	static void UnInstance();
public:
	GentFindMgr();
	~GentFindMgr();
	void Init();
	void ItemCreate(wchar_t *,size_t);
private:
	int NodesAdd(char *name,int index,int is_asc);
	void *Gmalloc(size_t size);
	void *Gcalloc(size_t size, int len);
	void Gfree(void *p);
	int Charwchar(char *str,wchar_t *out);
	size_t Wcstombs(char *buf,int buf_size,wchar_t *str); 	
	size_t Len(wchar_t *str);	

	node *NodeSet(int base,int check,int account,const char *name,short is_word);
	long GetEncode(const char *key, int base_val, int is_asc);
	void DelExQueue(int index);
	void AddExQueue(int index);
	void IncreMemary(int cur_len);
	int  GetChildCount(int parent_key);
	void GetChild(int parent_index,int ret[]);
	void SetChildCheck(int parent_index,int val);
	int GetBaseValue(int parent_index,int child_count,const char *key,int is_asc,int child[]);
	int MoveNode(int child_count,int child[],int real_base,int is_asc,int index);
	long NodesConflict(long encode_t, const char *name,int index,int is_asc);
};
#endif
