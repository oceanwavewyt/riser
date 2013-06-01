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


class GentLink
{
    static GentLink *intance_;
public:
	static GentLink *Instance();
	static void UnInstance();
public:
    GentLink();
    ~GentLink();
public:
    void Init();
private:
	void Createid(const string &quekey,string &);
};

#endif
