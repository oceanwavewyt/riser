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

#define hashsize(n) ((uint64_t)1<<(n))
#define hashmask(n) (hashsize(n)-1)

class HashInter
{
public:
    HashInter(){}
    ~HashInter(){}
protected:
    HashInter *successor;
    byte** tables;
public:
    virtual uint64_t Hash(char *) = 0;
    void Set(uint64_t pos) {
        
    }
    void Get() {
    
    }
    void Init() {
        tables = (byte **)malloc(hashsize(16)*sizeof(byte *));
    }
    void SetSuccessor(HashInter *s){
        successor = s;
    }
};


class SDBMHash : public HashInter
{
public:
    SDBMHash(){
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
        return (hash & 0x7FFFFFFF);
    }
};


class RSHash : public HashInter
{
public:
    RSHash(){}
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
        
        return (hash & 0x7FFFFFFF);
    }

};


class JSHash : public HashInter
{
public:
    JSHash(){
    
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
        
        return (hash & 0x7FFFFFFF);
    }
};

class GentList
{
    static GentList *intance_;
    list<HashInter *> hashList;
public:
	static GentList *Instance();
	static void UnInstance();
public:
    GentList();
    ~GentList();
public:
    void Save();
    void Load();
};

#endif
