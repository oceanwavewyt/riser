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
#define hashmask(n) (((uint64_t)1<<(n+3)))

const uint8_t posnum = 16;

class HashInter
{
public:
    HashInter(){}
    ~HashInter(){
        close(fd);
    }
private:
    int Position(char *key,bool isget=true) {
        uint64_t h = Hash(key);
        uint64_t pos = h/8;
        cout << "pos: "<< pos << endl;
        cout << "changdu:" << hashsize(posnum) << endl;
        int off = pos%8;
        int a[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
        if(isget) {
            return tables[pos] & a[off];
        }
        tables[pos] = tables[pos] | a[off];
        return 0;
    }
protected:
    HashInter *successor;
    byte* tables;
    string filename;
    int fd;
public:
    virtual uint64_t Hash(char *) = 0;
    void Set(char *key) {
        //tables[Hash(key)] = 0x01;
        int real = Position(key,false);
        if(real) {
            cout << "bloom set : true." << endl;
        }else{
            cout << "bloom set : false." << endl;
        }
        if(successor) {
            successor->Set(key);
        }
        msync((void*)tables,hashsize(posnum)*sizeof(byte),MS_ASYNC);
    }
    uint8_t Get(char *key, int parent) {
        //cout << filename << " key: " << key << " get Hash(key): " <<Hash(key) << endl;
        if(parent == -1) {
            parent = Position(key, true);
        }else{
            parent = parent & Position(key, true);
        }
        if(successor) {
            return successor->Get(key, parent);
        }
        return parent;
    }
    void Init() {
        if(access(filename.c_str(), 0) == -1) {
            cout << "access file not exist." << endl;
            tables = (byte *)malloc(hashsize(posnum)*sizeof(byte));
            memset(tables, 0, hashsize(posnum)*sizeof(byte));

            if ((fd = open(filename.c_str(), O_RDWR|O_CREAT,00777)) < 0){
                LOG(GentLog::ERROR, "%s open fail111.", filename.c_str());
                free(tables);
                exit(-1);
            }
            write(fd,tables, hashsize(posnum)*sizeof(byte));
            close(fd);
            free(tables);
        }
        
        if ((fd = open(filename.c_str(), O_RDWR)) < 0){
            LOG(GentLog::ERROR, "%s open fail.", filename.c_str());
            exit(-1);
        }
        
        struct stat sb; 
        if ((fstat(fd, &sb)) == -1) {
                LOG(GentLog::ERROR, "%s open fail.", filename.c_str());
                exit(-1);
        }
            
        if((tables = (byte *)mmap(NULL, sb.st_size, PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0))
                == (void *)-1){
            LOG(GentLog::ERROR, "mmap fail.");
            exit(-1);
        }
        
    }
    void SetSuccessor(HashInter *s){
        successor = s;
    }
};


class SDBMHash : public HashInter
{
public:
    SDBMHash(string &path){
        filename = path+"sdbm.dat";
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
