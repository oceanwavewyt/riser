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
};

typedef struct dataItem
{
	int sfd;
	char ip[50];
	int port;		
}dataItem;

class GentConnect
{
	int clen;
    GentCommand *comm;	    
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
	map<string, GentDestroy *> dest_list;
public:
    struct event ev;
    int fd;
	uint64_t start_time;
    struct sockaddr request_addr;
    bool is_slave;
	char ip[50];
	int port;
	int ev_flags;
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
	void RegDestroy(string &name, GentDestroy *d){
		map<string, GentDestroy*>::iterator it = dest_list.find(name);
		if(it != dest_list.end()) return;
		dest_list[name] = d;
	};
    void Init(int sfd);
	void SetAuth(int auth){comm->SetAuth(auth);};
	void SetClientData(dataItem *d);
private:
    int InitRead(int &rbytes);
	int ContinueRead(int &cbytes);
	void ResetConnect(); 
	int NextRead();
    void Reset();
    void ReAllocation();
};

#endif
