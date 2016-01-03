/*
 * gent_event.cpp
 *
 *  Created on: 2012-2-22
 *      Author: wyt
 */
#include "prefine.h"
#include "gentle.h"
#include "gent_event.h"
#include "gent_msg.h"
#include "gent_thread.h"
#include "gent_level.h"
#include "gent_app_mgr.h"

#define xisspace(c) isspace((unsigned char)c)



GentEvent *GentEvent::intance_ = NULL;

GentEvent *GentEvent::Instance() {
	if(intance_ == NULL) {
		intance_ = new GentEvent();
	}
	return intance_;
}

void GentEvent::UnIntance() {
	if(intance_) delete intance_;
}


GentEvent::GentEvent() {
	main_base_ = event_init();
}
GentEvent::~GentEvent() {
	//event_del(&ev_);
}

int GentEvent::AddEvent(GentConnect *conn,void(*handle)(const int fd, const short which, void *arg)) {
	LOG(GentLog::INFO, "create new connnect of %d,and init it", conn->fd);
	event_set(&conn->ev, conn->fd, eventRead, handle,conn);
	event_base_set(main_base_, &conn->ev);
	if(event_add(&conn->ev, 0) == -1) {
		LOG(GentLog::ERROR, "add event of %d failed", conn->fd);
		return -1;
	}
	return 0;
} 

int GentEvent::UpdateEvent(int fd,GentConnect *c, int state) {
    if(c->ev_flags == state) return 0;
	struct event_base *base = c->ev.ev_base;
	if(event_del(&c->ev)==-1){
		LOG(GentLog::ERROR, "delete event failed");	
        return -1;
    }
    LOG(GentLog::BUG, "GentEvent::Update set event %d", fd);
    event_set(&c->ev, fd, state, GentEvent::Handle,c);
	event_base_set(base, &c->ev);
	c->ev_flags = state;
	LOG(GentLog::BUG, "GentEvent::Update add event %d", fd);
	if(event_add(&c->ev, 0)==-1){
        LOG(GentLog::ERROR, "add event failed");
        return -1;
    }
	return 0;
}

int GentEvent::AddTimeEvent(struct timeval *tv, void(*handle)(const int fd, const short which, void *arg)) {
	//evtimer_set(&ev_, handle, this);
	event_set(&ev_, -1, EV_PERSIST, handle, this);
	event_base_set(main_base_, &ev_);
	event_add(&ev_, tv);
	return 0;
}

void GentEvent::Loop() {
	//while(true) {
	//	struct timeval tv;
	//	//每10毫秒执行一次
	//	tv.tv_sec = 0;
	//	tv.tv_usec = 1000;
	//	event_base_loopexit(main_base_, &tv);
	//	event_base_dispatch(main_base_);
	//	//GentRep::Intance()->Ret();
	//	if(GentFrame::Instance()->msg_.Cursize()>0){
	//	cout << GentFrame::Instance()->msg_.Cursize() << endl;
	//	}
	//}
	event_base_loop(main_base_,0);
}


void GentEvent::HandleMain(const int fd, const short which, void *arg) {
	int sfd, flags = 1;
	socklen_t addrlen;
	struct sockaddr_storage addr;
    addrlen = sizeof(addr);
    if ((sfd = accept(fd, (struct sockaddr *)&addr, &addrlen)) == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			/* these are transient, so don't log anything */
			return;
		} else if (errno == EMFILE) {
			LOG(GentLog::ERROR,"Too many open connections");
			return;
		} else {
			LOG(GentLog::WARN, "fd %d accept failed", fd);
			return;
		}

	}
    if ((flags = fcntl(sfd, F_GETFL, 0)) < 0 ||
   		fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        LOG(GentLog::ERROR, "setting O_NONBLOCK failed");
		perror("setting O_NONBLOCK");
        close(sfd);
        return;
    }
   	
    int nRecvBuf=32*1024;//设置为32K
    setsockopt(sfd,SOL_SOCKET,SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
    //发送缓冲区
    int nSendBuf=1*1024*1024;//设置为32K
    setsockopt(sfd,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int));

   	dataItem *d = (dataItem *)malloc(sizeof(dataItem));
	d->sfd = sfd;
	struct sockaddr_in sin;
 	memcpy(&sin, &addr, sizeof(sin));
	sprintf(d->ip, inet_ntoa(sin.sin_addr));	
	d->port = sin.sin_port; 
    if(!GentThread::Intance()->SendThread(d)) {
		free(d);
		LOG(GentLog::ERROR, "send thread data failed");
		close(sfd);
	}
}

void GentEvent::Handle(const int fd, const short which, void *arg) {
	GentConnect *c = static_cast<GentConnect *>(arg);
    if(c->fd != fd) {
		if(event_del(&c->ev) == -1) {
        	LOG(GentLog::ERROR, "event del fail");
		}
        	c->Destruct();
		GentAppMgr::Instance()->RetConnect(c);
        return;
	}
	string outstr;
    int readNum = c->TryRunning(outstr);
    if(readNum < 0) {
		if(event_del(&c->ev) == -1) {
			LOG(GentLog::ERROR, "event del fail");
		}
        c->Destruct();
        GentAppMgr::Instance()->RetConnect(c);
        return;
    }    
    
    //continue read
    
    //cout << c->rbuf << endl;
	
	//write(fd, buf.c_str(),buf.size());
    /*
	event_del(&c->ev);
	close(fd);
    free(c->rbuf);
	free(c);
     */
}


void GentEvent::Close(struct evhttp_connection *http_conn, void *args) {
	//cout << "GentEvent::Close"<< endl;
}


int GentEvent::Client(const string &host, int port)
{
	int client_sock=socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in address; 
	address.sin_addr.s_addr=inet_addr(host.c_str());    
	address.sin_family=AF_INET;  
	address.sin_port=htons(port);
	int result=connect(client_sock, (struct sockaddr *)&address, sizeof(address));  
	if(result==-1){  
    	LOG(GentLog::ERROR, "connect replicaton master failed");	
		close(client_sock);
		return -1;  
	} 	
	return client_sock; 
}
