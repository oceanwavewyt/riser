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
}

GentConnect::~GentConnect()
{
    close(fd);
	LOG(GentLog::INFO, "file description %d close.", fd);
    if(comm) {
        if(comm->rbuf)
            free(comm->rbuf);
        delete comm;
    }
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
                    comm->Reset();
                    return readNum;
                }else if(readNum == 0) {                                
                    return readNum;
                }
                remainsize = comm->Process(outstr);
                 if(!remainsize && outstr != "") {
                    //
                    curstatus = Status::CONN_WRITE;
                }
                break;
            case Status::CONN_NREAD:
                //OutString("ok\r\n"); 
                LOG(GentLog::INFO, "start conn_nread remainsize:%d",remainsize);
		if(!comm->content) {
                    new_rbuf = (char *)malloc(remainsize);
                    memset(new_rbuf,0,remainsize);
                    comm->rcont = comm->content = new_rbuf;
                }
                if(NextRead() == -1) {
                    stop = 1;
                    break;		
                }
                break;
            case Status::CONN_DATA:
                outstr = "";
                comm->Complete(outstr);
                curstatus = Status::CONN_WRITE;	
                break;
            case Status::CONN_WRITE:
                OutString(outstr);
                curstatus = Status::CONN_WAIT;
                break;
            case Status::CONN_WAIT:
				LOG(GentLog::INFO, "the status of %d is connect wait", fd);
                remainsize = 0;
                comm->Reset();
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
        if (comm->rbytes >= comm->rsize) {
            if (num_allocs == 4) {                                       
                return gotdata;                                          
            }                                                            
            ++num_allocs;                                                
            char *new_rbuf = (char *)realloc(comm->rbuf, comm->rsize * 2);
            if (!new_rbuf) {                                             
                LOG(GentLog::ERROR, "Couldn't realloc input buffer");
                comm->rbytes = 0; /* ignore what we read */
                return -2;                                               
            } 
            comm->rbuf = new_rbuf;
 //           c->rcurr = c->rbuf = new_rbuf;                               
            comm->rsize *= 2;
        }                                                                
                                                                         
        int avail = comm->rsize - comm->rbytes;
        res = read(fd, comm->rbuf + comm->rbytes, avail);                   
        if (res > 0) {                                                   
            //pthread_mutex_lock(&c->thread->stats.mutex);               
            //c->thread->stats.bytes_read += res;                        
            //pthread_mutex_unlock(&c->thread->stats.mutex);             
            gotdata = 1;                                                 
            comm->rbytes += res;
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
	if (comm->rbytes > 0) {                                             
		int tocopy = comm->rbytes > remainsize?remainsize : comm->rbytes;
		LOG(GentLog::WARN, "tocopy: %d",tocopy);
			memmove(comm->rcont, comm->rcurr, tocopy);
		remainsize -= tocopy;                                        
		comm->rbytes -= tocopy;                                         
		comm->rcont += tocopy;
        comm->rcurr = comm->rcont;
		if (remainsize == 0) {                                       
			SetStatus(Status::CONN_DATA);                                                   
			return 0;
		}                                                            
	}                                                                
	int res;
	/*  now try reading from the socket */                           
	res = read(fd, comm->rcont, remainsize);
	if (res > 0) {
		comm->rcont += res;                                             
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
