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
#include "gent_app_mgr.h"
#include "gent_repl.h"
#include <errno.h>
GentConnect::GentConnect(int sfd):rbuf(NULL)
{
	Init(sfd);
	
	st_map[Status::CONN_READ] = "read"; 
	st_map[Status::CONN_NREAD] = "read"; 
	st_map[Status::CONN_WRITE] = "send";
	st_map[Status::CONN_WAIT] = "wait"; 
	st_map[Status::CONN_CLOSE] = "close";
	st_map[Status::CONN_DATA] = "parse";
	st_map[Status::CONN_CONREAD] = "continue_read";
}

GentConnect::~GentConnect()
{
    Destruct();
}

void GentConnect::SetClientData(dataItem *d)
{
	memcpy(ip, d->ip, 50);
	port = d->port;
}

void GentConnect::Destruct()
{
    if(rbuf) {
        free(rbuf);
        rbuf = NULL;
    }
    if(content) {
        free(content);
        content = NULL;
    }
	start_time = 0;	
    if(fd<0) return;
	set<GentDestroy*>::iterator it;
	for(it=dest_list.begin();it!=dest_list.end();it++) {
		(*it)->Destroy();
	}
	dest_list.clear();
    GentAppMgr::Instance()->Destroy(fd);
	close(fd);
	curstatus = Status::CONN_CLOSE;
	LOG(GentLog::BUG, "file description %d close.", fd);
   	comm = NULL; 
}
void GentConnect::Init(int sfd) {
    fd = sfd;
	is_slave = false;
	start_time = time(NULL);
    clen = 0;
    remainsize = 0;
	sendsize = 0;
	cursendsize = 0;
	csize = 0;
	cbytes = 0;
	cbuf = NULL;
	ev_flags = eventRead; 
    curstatus = Status::CONN_READ;
    //configure info
    comm = GentAppMgr::Instance()->GetCommand(this, fd);
    ReAllocation();
}

void GentConnect::ReAllocation() {
	//rcurr = NULL;
	content = NULL;
	actualsize = 0;
    if(!rbuf) {
        rsize = GentCommand::READ_BUFFER_SIZE;
        rbuf = (char *)malloc(rsize);
    }
    memset(rbuf,0,rsize);
}

void GentConnect::Reset() {
    //comm->Reset();
    //if(rbuf) {
    //    free(rbuf);
    //}
    if(content) {
        free(content);
		content = NULL;
    }
	if(cbuf) {
		free(cbuf);
		cbuf = NULL;
	}
    ReAllocation();
}

int GentConnect::TryRunning(string &outstr2) {
    int rbytes = 0;
    int readNum;
    int stop=0;
	while(true) {
	  if(stop==1) break;
        switch(curstatus) {
            case Status::CONN_READ:
                outstr = "";
                readNum = InitRead(rbytes);
                LOG(GentLog::BUG, "init read fd:%d the number of byte is %d.", fd, readNum);
				if(readNum < 0) {
                    LOG(GentLog::BUG, "init read the number of byte less than zero");
                    outstr2 = "read error\r\n";
                    Reset();
                    return readNum;
                }else if(readNum == 0) {
					curstatus = Status::CONN_WAIT;                                
                    break;
                }
                remainsize = comm->Process(rbuf, rbytes, outstr);
                if(!remainsize && outstr != "") {
                     curstatus = Status::CONN_WRITE;
                     Reset();
		     		 LOG(GentLog::BUG, "remainsize false, outstr!=null updateevent.");
                     if(GentEvent::UpdateEvent(fd, this, eventWrite)==-1) {
						return -1;
		     		}
		     		return 0;
                }
                break;
            case Status::CONN_NREAD:
                //LOG(GentLog::INFO, "start conn_nread remainsize:%d",remainsize);
				if(!content) {
                    content = (char *)malloc(remainsize);
                    memset(content,0,remainsize);
                    rcont = content;
                }
                if(NextRead() == -1) {
                    stop = 1;
                    break;		
                }
                break;
            case Status::CONN_DATA:
                outstr = "";
                comm->Complete(outstr,content, actualsize);
                if(outstr != "") {
					curstatus = Status::CONN_WRITE;
					LOG(GentLog::BUG, "conn_data updateevent");
                	if(GentEvent::UpdateEvent(fd, this, eventWrite)==-1) {
						return -1;
					}
                }
                return 0;
            case Status::CONN_WRITE:
                Reset();
				OutString(outstr);
                //return 0;
                break;
            case Status::CONN_WAIT:
				LOG(GentLog::BUG, "the status of %d is connect wait", fd);
                remainsize = 0;
                Reset();
                curstatus = Status::CONN_READ;
				LOG(GentLog::BUG, "conn_wait updateevent");
                if(GentEvent::UpdateEvent(fd, this, eventRead)==-1) {
					return -1;	
				}
                
                return 0;
			case Status::CONN_CONREAD:
				if(!cbuf) {
					csize = GentCommand::READ_BUFFER_SIZE;
		        	cbuf = (char *)malloc(csize);
					memset(cbuf,0, csize);
				}
				readNum = ContinueRead(cbytes);
				if(readNum < 0) {
                    LOG(GentLog::BUG, "continue read the number of byte less than zero");
                    outstr2 = "read error\r\n";
                    Reset();
                    return readNum;
                }else if(readNum == 0) {
					//wait                               
                   	if(GentEvent::UpdateEvent(fd, this, eventRead)==-1) {
						return -1;	
					}
					return 0; 
                }
				if(!comm->ContinueProcess(cbuf, cbytes, outstr)) {
					free(cbuf);
					csize = 0;
					cbytes = 0;
					cbuf = NULL;
				}else{
					memset(cbuf,0 ,csize);
					cbytes = 0;
					GentEvent::UpdateEvent(fd, this, eventRead);
					return 0;
				}
				break;
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
        res = read(fd, rbuf + rbytes, avail);                   
		if (res > 0) {                                                   
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

int GentConnect::OutString(const string &str) {
	if(sendsize <= 0) {
		sendsize = str.size();
		cursendsize = 0;
	}
	char *curpos = const_cast<char *>(str.c_str());	
	int slen;
	while(sendsize>0) {
    	slen = send(fd, curpos+cursendsize, sendsize, 0);
		if (slen == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			LOG(GentLog::BUG, "OutString updateevent.");	
			GentEvent::UpdateEvent(fd, this, eventWrite);
			return 0;	
		}
		if(slen<0) return -1;
		sendsize -= slen;
		cursendsize += slen;
	}
    curstatus = Status::CONN_WAIT;
	return cursendsize;
}

void GentConnect::SetWrite(const string &str)
{
	outstr = str;
	curstatus = Status::CONN_WRITE;
	LOG(GentLog::BUG, "SetWrite updateevent");
    	GentEvent::UpdateEvent(fd, this, eventWrite);
}

void GentConnect::SetStatus(int s) {
    curstatus = s;
}

string GentConnect::GetStatus() {
	return st_map[curstatus];			
}

int GentConnect::NextRead() {
	if (remainsize <= 0) {
		SetStatus(Status::CONN_DATA);
		return 0;		
	}
                                                              
	int res;
	/*  now try reading from the socket */                           
	res = read(fd, rcont, remainsize);
	if (res > 0) {
		rcont += res;                                             
		remainsize -= res;
		actualsize += res;                                           
		return 0;                                  
	}                                                                
	if (res == 0) { /* end of stream */                             
	    SetStatus(Status::CONN_CLOSE);
	    return 0;
	}                                                               
	if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) { 
	LOG(GentLog::BUG, "NextRead updateevent");  
        GentEvent::UpdateEvent(fd, this, eventRead);
		return -1;                                                            
	}
	SetStatus(Status::CONN_CLOSE);                                                               
	return 0;
}

int GentConnect::ContinueRead(int &cbytes) {                         
    int gotdata = 0;                                                     
    int res;                                                             
    int num_allocs = 0;                                                  
 	                                                                      
    while (1) {                                                          
        if (cbytes >= csize) {
            if (num_allocs == 4) {                                       
                return gotdata;                                          
            }                                                            
            ++num_allocs;                                                
            char *new_rbuf = (char *)realloc(cbuf, csize * 2);
            if (!new_rbuf) {                                             
                LOG(GentLog::ERROR, "Couldn't realloc input buffer");
                rbytes = 0; /* ignore what we read */
                return -2;                                               
            } 
            cbuf = new_rbuf;                               
            csize *= 2;
        }                                                                
                                                                         
        int avail = csize - cbytes;
        res = read(fd, cbuf + cbytes, avail);                   
		if (res > 0) {                                                   
            gotdata = 1;                                                 
            cbytes += res;
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
		LOG(GentLog::BUG, "continue updateevent");      
	        GentEvent::UpdateEvent(fd, this, eventRead); 
               break;                                       
           }
           return -1;
       }                                                    
   }
   return gotdata;                                          
}
