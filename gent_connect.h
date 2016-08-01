//
//  gent_connect.h
//  riser
//
//  Created by wyt on 13-1-27.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#ifndef riser_gent_connect_h
#define riser_gent_connect_h
#include <event.h>
#include "prefine.h"
#include "gent_command.h"
#include <set>

class GentEvent;

class Status
{
public:
    enum connst
    {
        CONN_READ = 1,
        CONN_NREAD = 2,
        CONN_WRITE = 3,
        CONN_WAIT = 4,
        CONN_CLOSE = 5,
		CONN_DATA = 6,
		CONN_CONREAD = 7
    };
};



typedef struct dataItem
{
	int sfd;
	char ip[50];
	int port;		
}dataItem;
class GentDestroy;
class GentConnect
{
	int clen;
    char *rbuf;
    char *rcurr;
    char *content;
    char *rcont;
    int rsize;
    int rbytes;

	char *cbuf;
	int csize;
	int cbytes;
    string outstr;

	uint64_t sendsize;
	uint64_t cursendsize;    
    uint32_t curstatus;
	uint64_t remainsize;
	uint64_t actualsize;
   	map<int,string> st_map;
	std::set<GentDestroy*> dest_list;
public:
    struct event ev;
    int fd;
	uint64_t start_time;
    struct sockaddr request_addr;
    bool is_slave;
	char ip[50];
	int port;
	int ev_flags;
    GentCommand *comm;	    
public:
    GentConnect(int);
    ~GentConnect();
public:
    int OutString(const string &str);
    int TryRunning(string &str);
    void SetStatus(int);
    string GetStatus();
	void SetWrite(const string &str);
	void Destruct();
	void RegDestroy(GentDestroy *d){
		dest_list.insert(d);
	};
    void Init(int sfd);
	void SetAuth(int auth){comm->SetAuth(auth);};
	void SetClientData(dataItem *d);
    void Reset();
private:
    int InitRead(int &rbytes);
	int ContinueRead(int &cbytes);
	void ResetConnect(); 
	int NextRead();
    void ReAllocation();
};

class GentDestroy
{
protected:
	GentConnect *conn_;
public:
    GentDestroy(){};
    virtual ~GentDestroy(){};
public:
    void Destroy(){
		conn_ = NULL;
	};
	GentConnect *GetConn(){
		return conn_;
	}; 
	void SetConn(GentConnect *c) {
		conn_ = c;
		conn_->RegDestroy(this);
	};
};

#endif
