//
//  gent_connect.cpp
//  riser
//
//  Created by wyt on 13-1-27.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#include "gent_connect.h"
#include "gent_event.h"
#include "gent_level.h"

GentConnect::GentConnect(int sfd)
{
    fd = sfd;
    clen = 0;
    remainsize = 0;
    //configure info
    comm = new GentLevel(this);
    curstatus = Status::CONN_READ;
	Init();
}

GentConnect::~GentConnect()
{
    close(fd);
	LOG(GentLog::INFO, "file description %d close.", fd);
    if(rbuf) {
        free(rbuf);
    }
    if(comm) {
        delete comm;
    }
}

void GentConnect::Init() {
    rsize = GentCommand::READ_BUFFER_SIZE;
    cout << "rsize: " << rsize << endl;
	rcurr = NULL;
    content = NULL;
    rbuf = (char *)malloc(rsize);
    memset(rbuf,0,rsize);
}

void GentConnect::Reset() {
    comm->Reset();
    if(rbuf) {
        free(rbuf);
    }
    if(content) {
        free(content);
    }
    Init();
}

int GentConnect::TryRunning(string &outstr) {
    outstr = "";
    char *new_rbuf;
    int rbytes = 0;
    int readNum;
    int stop=0;
	while(true) {
	  if(stop==1) break;
        switch(curstatus) {
            case Status::CONN_READ:
                readNum = InitRead(rbytes);
                LOG(GentLog::INFO, "init read the number of byte is %d.", readNum);
                if(readNum < 0) {
                    LOG(GentLog::WARN, "init read the number of byte less than zero");
                    outstr = "read error\r\n";
                    //printf("read error num:%d\n",readNum);
                    Reset();
                    return readNum;
                }else if(readNum == 0) {                                
                    return readNum;
                }
				//cout <<"rbtyes:" << rbytes << " return rbuf: "<< rbuf << endl;
                remainsize = comm->Process(rbuf, rbytes, outstr);
                 if(!remainsize && outstr != "") {
                    //
                    curstatus = Status::CONN_WRITE;
                }
                break;
            case Status::CONN_NREAD:
                //OutString("ok\r\n"); 
                LOG(GentLog::INFO, "start conn_nread remainsize:%d",remainsize);
				if(!content) {
                    new_rbuf = (char *)malloc(remainsize);
                    memset(new_rbuf,0,remainsize);
                    rcont = content = new_rbuf;
                }
                if(NextRead() == -1) {
                    stop = 1;
                    break;		
                }
                break;
            case Status::CONN_DATA:
                outstr = "";
                comm->Complete(outstr,content);
                curstatus = Status::CONN_WRITE;	
                break;
            case Status::CONN_WRITE:
                OutString(outstr);
                curstatus = Status::CONN_WAIT;
                break;
            case Status::CONN_WAIT:
				LOG(GentLog::INFO, "the status of %d is connect wait", fd);
                remainsize = 0;
                Reset();
                //GentEvent::Instance()->UpdateEvent(fd, this);
                gevent->UpdateEvent(fd, this);
                curstatus = Status::CONN_READ;
                return 0;
            case Status::CONN_CLOSE:
                return -1;		
            default:
                return -1;
        }
        
	}
	return 1;
}

int GentConnect::InitRead(int &rbytes) {                         
    int gotdata = 0;                                                     
    int res;                                                             
    int num_allocs = 0;                                                  
 	                                                                      
    while (1) {                                                          
        if (rbytes >= rsize) {
            if (num_allocs == 4) {                                       
                return gotdata;                                          
            }                                                            
            ++num_allocs;                                                
            char *new_rbuf = (char *)realloc(rbuf, rsize * 2);
            if (!new_rbuf) {                                             
                LOG(GentLog::ERROR, "Couldn't realloc input buffer");
                rbytes = 0; /* ignore what we read */
                return -2;                                               
            } 
            rbuf = new_rbuf;                               
            rsize *= 2;
        }                                                                
                                                                         
        int avail = rsize - rbytes;
		cout << "avail: " << avail << endl;
        res = read(fd, rbuf + rbytes, avail);                   
        cout << "res: " << res << endl;
		if (res > 0) {                                                   
            //pthread_mutex_lock(&c->thread->stats.mutex);               
            //c->thread->stats.bytes_read += res;                        
            //pthread_mutex_unlock(&c->thread->stats.mutex);             
            gotdata = 1;                                                 
            rbytes += res;
            if (res == avail) {                                          
                continue;                                                
            } else {                                                     
                break;                                                   
            }                                                            
        }                                                                
       if (res == 0) {                                      
           return -3;                                       
       }                                                    
       if (res == -1) {                                     
           if (errno == EAGAIN || errno == EWOULDBLOCK) {   
               break;                                       
           }                                                
           return -1;                                       
       }                                                    
   }                                                        
   return gotdata;                                          
}

void GentConnect::OutString(const string &str) {
    write(fd, str.c_str(),str.size());
}

void GentConnect::SetStatus(int s) {
    curstatus = s;
}

int GentConnect::NextRead() {
	if (remainsize <= 0) {
		SetStatus(Status::CONN_DATA);
		return 0;		
	}
	/* first check if we have leftovers in the conn_read buffer */   
/*
	if (rbytes > 0) {                                             
		int tocopy = rbytes > remainsize?remainsize : rbytes;
		LOG(GentLog::WARN, "tocopy: %d",tocopy);
			memmove(rcont, rcurr, tocopy);
		remainsize -= tocopy;                                        
		rbytes -= tocopy;                                         
		rcont += tocopy;
        rcurr = rcont;
		if (remainsize == 0) {                                       
			SetStatus(Status::CONN_DATA);                                                   
			return 0;
		}                                                            
	}
*/                                                                
	int res;
	/*  now try reading from the socket */                           
	res = read(fd, rcont, remainsize);
	if (res > 0) {
		rcont += res;                                             
		remainsize -= res;                                           
		return 0;                                  
	}                                                                
	if (res == 0) { /* end of stream */                             
	    SetStatus(Status::CONN_CLOSE);
	    return 0;
	}                                                               
	if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {   
        gevent->UpdateEvent(fd, this);
    //		GentEvent::Instance()->UpdateEvent(fd, this);
	//	if (!update_event(c, EV_READ | EV_PERSIST)) {               
        //		conn_set_state(c, conn_closing);                        
        // 		break;                                                  
    	//	}
		return -1;                                                            
	
	}
	SetStatus(Status::CONN_CLOSE);                                                               
	return 0;
}