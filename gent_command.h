//
//  gent_command.h
//  riser
//
//  Created by wyt on 13-1-27.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#ifndef riser_gent_command_h
#define riser_gent_command_h
#include "prefine.h"
//#include "gent_connect.h"
class GentConnect;

class GentCommand
{
protected:
    GentConnect *conn;
public:
    //char *rbuf;
    //char *rcurr;
	//char *rcont;
    int rsize;
    int rbytes;
public:
    GentCommand(GentConnect *c);
    ~GentCommand();
public:
    virtual int Process(const char *rbuf, uint64_t size, string &outstr) = 0;
    virtual void Complete(string &outstr, const char *recont, uint64_t len) = 0;
    virtual GentCommand *Clone(GentConnect *) = 0;
	virtual bool Init(string &msg) = 0;
	virtual void Reset();
public:
    static const int READ_BUFFER_SIZE = 1024;
};

class GentWang
{
public:
    virtual bool Init(string &)=0;
};
#endif
