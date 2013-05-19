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

const uint8_t posnum = 32;

class HashInter
{
public:
    HashInter(){}
    ~HashInter(){}
protected:
    HashInter *successor;
    byte* tables;
public:
    virtual uint64_t Hash(char *) = 0;
    void Set(char *key) {
        tables[Hash(key)] = 0x01;
        cout << "key: " << key << "set Hash(key): " <<Hash(key) << endl;
        if(tables[Hash(key)]) {
            cout << "bloom set : true." << endl;
        }else{
            cout << "bloom set : false." << endl;
        }
        if(successor) {
            successor->Set(key);
        }
    }
    uint8_t Get(char *key, int parent) {
        cout << "key: " << key << " get Hash(key): " <<Hash(key) << endl;
        if(parent == -1) {
            parent = tables[Hash(key)];
        }else{
            parent = parent & tables[Hash(key)];
        }
        if(successor) {
            return successor->Get(key, parent);
        }
        return parent;
    }
    void Init() {
        tables = (byte *)malloc(hashsize(posnum)*sizeof(byte));
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
        return (hash & 0x7FFFFFFF) & hashmask(posnum);
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
        
        return (hash & 0x7FFFFFFF) & hashmask(posnum);
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
    void Load(string &k);
};

#endif
