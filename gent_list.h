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

#define hashsize(n) ((uint64_t)1<<(n))
#define hashmask(n) ((hashsize(n+3))-1)

const uint8_t posnum = 16;

class HashInter
{
	uint8_t posval[8];
    CommLock hash_lock;
public:
    HashInter();
    ~HashInter();
private:
    uint8_t Position(char *key,bool isget=true);
protected:
    HashInter *successor;
    byte* tables;
    string filename;
    int fd;
public:
    virtual uint64_t Hash(char *) = 0;
    void Set(char *key);
    int Get(char *key, int parent);    
	void Init();
    void SetSuccessor(HashInter *s);
};


class SDBMHash : public HashInter
{
public:
    SDBMHash(string &path){
        filename = path+"sdbm.dat";
    	successor = NULL;
	}
    ~SDBMHash(){
    }
public:
    uint64_t Hash(char *str)
    {
        uint64_t hash = 0;
        
        while (*str)
        {
            hash = (*str++) + (hash << 6) + (hash << 16) - hash;
        }
        return (hash & 0x7FFFFFFF) & hashmask(posnum);
    }
};


class RSHash : public HashInter
{
public:
    RSHash(string &path){
        filename = path+"rs.dat";
		successor = NULL;
    }
    ~RSHash(){}
    uint64_t Hash(char *str)
    {
        uint64_t b = 378551;
        uint64_t a = 63689;
        uint64_t hash = 0;
        
        while (*str)
        {
            hash = hash * a + (*str++);
            a *= b;
        }
        
        return (hash & 0x7FFFFFFF) & hashmask(posnum);
    }

};


class JSHash : public HashInter
{
public:
    JSHash(string &path){
        filename = path+"js.dat";
		successor = NULL;
    }
    ~JSHash(){
    
    }
    uint64_t Hash(char *str)
    {
        uint64_t hash = 1315423911;
        
        while (*str)
        {
            hash ^= ((hash << 5) + (*str++) + (hash >> 2));
        }
        
        return (hash & 0x7FFFFFFF)  & hashmask(posnum);
    }
};

class GentList
{
    static GentList *intance_;
    list<HashInter *> hashList;
    HashInter *mainHash;
public:
	static GentList *Instance();
	static void UnInstance();
public:
    GentList();
    ~GentList();
public:
    void Init();
    void Save(string &k);
    int Load(string &k);
};

#endif
