/*
 * gent_thread.cpp
 *
 *  Created on: 2012-6-15
 *      Author: wyt
 */
#include "prefine.h"
#include <pthread.h>
#include "gentle.h"
#include "gent_thread.h"
#include "gent_app_mgr.h"
#include "gent_connect.h"
#include "gent_repl.h"
#include "gent_db.h"

GentThread *GentThread::intanceth_ = NULL;

GentThread *GentThread::Intance() {
	if(intanceth_ == NULL) {
		intanceth_ = new GentThread();
	}
	return intanceth_;
}

void GentThread::UnIntance() {
	if(intanceth_) delete intanceth_;
}


GentThread::GentThread():thread_count_(1){
	lastid_ = 0;
	//base_ = event_init();
}

GentThread::~GentThread(){

}
 
void GentThread::init(int fd, int thread_count) {
	thread_count_ = thread_count+1;
	threads_ = (THREADINFO *)malloc(thread_count_ * sizeof(THREADINFO));
	memset(threads_, 0, thread_count_ * sizeof(THREADINFO));	
	threads_[0].id = 0;
	threads_[0].base = event_init();
	event_set(&threads_[0].event, fd, eventRead, GentEvent::HandleMain, NULL);                
	event_base_set(threads_[0].base, &threads_[0].event);                                 
	if(event_add(&threads_[0].event, 0) == -1) {                                    
    		LOG(GentLog::INFO, "add main event of %d failed", fd);            
    		return;                                                         
	}

	int i;
    LOG(GentLog::INFO, "init child thread");
	for(i=1; i<thread_count_; ++i) {
		int fd[2];
		if(pipe(fd)) {
			LOG(GentLog::ERROR,"thread initialize failed");
			exit(1);
		}
		threads_[i].id = i;
		threads_[i].rec_id = fd[0];
		threads_[i].send_id = fd[1];
		SetupThread(&threads_[i]);
	}
}
void GentThread::SetupThread(THREADINFO *thread) {
	//thread->gevent = new GentEvent();
	thread->base = event_init();
	event_set(&thread->event, thread->rec_id, eventRead, GentThread::Handle, thread);                
	event_base_set(thread->base, &thread->event);                                 
	if(event_add(&thread->event, 0) == -1) {                                    
    		LOG(GentLog::INFO, "add event of %d failed", thread->rec_id);            
    		return;                                                         
	}                                                                      

}

bool GentThread::SendThread(dataItem *d) {
	int tid = (lastid_+1)%thread_count_;
	if(tid == 0) tid = 1;
    GentFrame::Instance()->msg_[tid].Push(d);	
	lastid_ = tid;
	if (write(threads_[tid].send_id, "", 1) != 1) {
	        perror("Writing to thread notify pipe");
		return false;
	}
	return true;
}

void *GentThread::Work(void *arg) {
	THREADINFO *thread = static_cast<THREADINFO *>(arg);
    LOG(GentLog::INFO, "start the %d thread's %ld", thread->id, pthread_self());
	event_base_loop(thread->base,0);
	return ((void *)0);
}

void *GentThread::Rep(void *arg) {
	GentEvent *ev = new GentEvent();
	struct timeval tv = {1,0};
	ev->AddTimeEvent(&tv, GentRepMgr::SlaveHandle);
	ev->Loop(); 
	return ((void *)0);
}

void GentThread::Start() {
	int i;
	for(i=1; i<thread_count_; ++i) {
		pthread_attr_t attr;
		pthread_t pid;
		int ret;
		pthread_attr_init(&attr);
		ret = pthread_create(&pid,&attr,GentThread::Work,&threads_[i]);
		if(ret != 0) {
			INFO(GentLog::ERROR,"thread create failed");
			exit(0);
		}

	}
	GentConfig &config = GentFrame::Instance()->config;
	if(config["slaveof_ip"] != "" && config["slaveof_port"] != ""){
		pthread_attr_t attr;
		pthread_t pid;
		int ret;
		pthread_attr_init(&attr);
		ret = pthread_create(&pid,&attr,GentThread::Rep,NULL);
		if(ret != 0) {
			INFO(GentLog::ERROR,"thread create failed");
			exit(0);
		}		
	}
	INFO(GentLog::INFO,"start successful");	
	event_base_loop(threads_[0].base,0);
}

bool GentThread::StartClear() {
	pthread_attr_t attr;
	pthread_t pid;
	int ret;
	pthread_attr_init(&attr);
	ret = pthread_create(&pid,&attr,GentThread::ClearHandle,NULL);
	if(ret != 0) {
		LOG(GentLog::ERROR,"clear thread create failed");
		return false;
	}
	return true;
}

void *GentThread::ClearHandle(void *arg) {
	uint64_t clearNum = GentDb::Instance()->ClearExpireKey();	
	LOG(GentLog::INFO,"clear key num: %llu", (unsigned long long)clearNum);	
	return ((void *)0);
}

void GentThread::Handle(int fd, short which, void *arg) {
	char buf[2];
	THREADINFO *me = (THREADINFO *)arg;
	 if (read(fd, buf, 1) != 1){
         LOG(GentLog::WARN, "Can't read from libevent pipe");
	 }
	//    GentConnect *nconn = GentFrame::Instance()->msg_.Pop();
    dataItem *item = GentFrame::Instance()->msg_[me->id].Pop();
	GentConnect *c = GentAppMgr::Instance()->GetConnect(item->sfd);
  	free(item);	 
    LOG(GentLog::BUG, "start deal new connection fd:%d", c->fd);
	event_set(&c->ev, c->fd, eventRead ,GentEvent::Handle , (void *)c);
    event_base_set(me->base, &c->ev);
    c->ev_flags = eventRead;
    if (event_add(&c->ev, 0) == -1) {
		cout << "add nconn failed" <<endl;
		LOG(GentLog::ERROR, "GentThread::Handle add event failed");
		c->Destruct();
        GentAppMgr::Instance()->RetConnect(c);
    }
}

