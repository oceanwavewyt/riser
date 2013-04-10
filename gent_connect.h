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
		CONN_DATA = 6
    };
};

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



    int curstatus;
	int remainsize;
	uint64_t actualsize;
public:
    struct event ev;
    GentEvent *gevent;
    int fd;
public:
    GentConnect(int);
    ~GentConnect();
public:
    void OutString(const string &str);
    int TryRunning(string &str);
    void SetStatus(int);
private:
    int InitRead(int &rbytes);
	void ResetConnect(); 
	int NextRead();
    void Init();
    void Reset();
};

#endif
