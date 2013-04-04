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


GentThread::GentThread():thread_count_(10){
	lastid_ = 0;
	//base_ = event_init();
}

GentThread::~GentThread(){

}
void GentThread::init(int thread_count) {
	thread_count_ = thread_count;
	int i;
    LOG(GentLog::INFO, "init child thread");
	for(i=0; i<thread_count_; ++i) {
//		cout <<"GentThread::init:  " <<  i << endl;
		int fd[2];
		if(pipe(fd)) {
			cout << "[GentThread] init pipe failed" << endl;
			exit(1);
		}
		threads_[i].id = i;
		threads_[i].rec_id = fd[0];
		threads_[i].send_id = fd[1];
		threads_[i].th = this;
        threads_[i].gevent= new GentEvent();
		//threads_[i].evbase = base_;
		//threads_[i].evbase = event_init();
		SetupThread(&threads_[i]);
	}
}
void GentThread::SetupThread(THREADINFO *thread) {
	//event_set(&thread->event_, thread->rec_id, EV_READ | EV_PERSIST,GentThread::Handle,thread);
	//event_base_set(thread->evbase, &thread->event_);
	//event_add(&thread->event_, 0);
    GentConnect *conn = new GentConnect(thread->rec_id);
    conn->gevent = thread->gevent;
    thread->gevent->AddEvent(conn, GentThread::Handle);
}

void GentThread::SendThread() {
	int tid = (lastid_+1)%thread_count_;
	lastid_ = tid;
	if (write(threads_[tid].send_id, "", 1) != 1) {
	        perror("Writing to thread notify pipe");
	}
}

void *GentThread::Work(void *arg) {
	THREADINFO *thread = static_cast<THREADINFO *>(arg);
    LOG(GentLog::INFO, "start the %d thread's %ld", thread->id, pthread_self());
    thread->gevent->Loop();
//	while(true) {
//		GentThread::Intance()->Handle2(thread);
//	}
//	while(true) {
//		struct timeval tv;
//		//每10毫秒执行一次
//		tv.tv_sec = 0;
//		tv.tv_usec = 1000;
//		event_base_loopexit(thread->evbase, &tv);
//		event_base_dispatch(thread->evbase);
		//GentRep::Intance()->Ret();

//	}

//	event_base_loop(thread->evbase,0);
	return ((void *)0);
}

void GentThread::Start() {
	int i;
	for(i=0; i<thread_count_; ++i) {
		pthread_attr_t attr;
		pthread_t pid;
		int ret;
		pthread_attr_init(&attr);
		ret = pthread_create(&pid,&attr,GentThread::Work,&threads_[i]);
		//ret = pthread_create(&pid,&attr,GentThread::Handle2,&threads_[i]);
		if(ret != 0) {
			cout << "[GentThread] Start pthread create failed." << endl;
			exit(0);
		}

	}

}

void *GentThread::Handle2(void *arg) {
	//if(GentFrame::Instance()->msg_.Cursize()==0) return;
//	THREADINFO *thread = static_cast<THREADINFO *>(arg);

	//GentThread *gthread = static_cast<GentThread *>(thread->th);
//	GentBasic *app = NULL;
	cout << "GentThread::Handle " << pthread_self()<< endl;
	while(true) {
		//cout << "abc " << pthread_self()<< endl;
		GentConnect *pack2 = GentFrame::Instance()->msg_.Pop();
//        LOG(GentLog::INFO, "pack pop");
        GentEvent::Instance()->AddEvent(pack2, GentEvent::Handle);
		//cout << "abc2 " << pthread_self()<< endl;
//		if(GentAppMgr::Intance()->GetModule(1,app)>0) {
//			printf("GetModule failed\n");
//		}else{
//			COMM_REP rep={0};
//			rep.request_ = pack2.request_;
//			if(app->Proccess()==0) {
//				memcpy(rep.ret_buff_,app->resp_str_.c_str(),app->resp_str_.size());
//			}
//			GentAppMgr::Intance()->SetModule(1,app);
			//GentRep::Intance()->rep_.Push(rep);
			//GentRep::Intance()->Ret(rep);
//		}
	}
	return ((void *)0);
}

void GentThread::Handle(int fd, short which, void *arg) {
	char buf[2];
	//THREADINFO *thread = static_cast<THREADINFO *>(arg);
    GentConnect *c = static_cast<GentConnect *>(arg);
	 if (read(fd, buf, 1) != 1){
         LOG(GentLog::WARN, "Can't read from libevent pipe");
	 }
    GentConnect *nconn = GentFrame::Instance()->msg_.Pop();
    nconn->gevent = c->gevent;
    LOG(GentLog::INFO, "start deal new connection");
    nconn->gevent->AddEvent(nconn, GentEvent::Handle);
    
	//GentThread *gthread = static_cast<GentThread *>(thread->th);
//	cout << "GentThread::Handle " << pthread_self()<< endl;
//    string ret = "ok";
//    write(fd,ret.c_str(),ret.size());
//    close(fd);
}

