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
	event_add(&conn->ev, 0);
	return 0;
} 

int GentEvent::UpdateEvent(int fd,GentConnect *c, int state) {
    if(event_del(&c->ev)==-1){
        cout << "event_del failed" << endl;
        return -1;
    }
    event_set(&c->ev, fd, state, GentEvent::Handle,c);
	event_base_set(main_base_, &c->ev);
	if(event_add(&c->ev, 0)==-1){
        cout << "event_add failed" << endl;
        return -1;
    }
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
    

    //GentConnect *gconnect = new GentConnect(sfd);
    GentConnect *gconnect = GentAppMgr::Instance()->GetConnect(sfd);
    GentFrame::Instance()->msg_.Push(gconnect);
    GentThread::Intance()->SendThread();
}

void GentEvent::Handle(const int fd, const short which, void *arg) {
	GentConnect *c = static_cast<GentConnect *>(arg);
    string outstr;
    int readNum = c->TryRunning(outstr);
    if(readNum < 0) {
        event_del(&c->ev);
        GentAppMgr::Instance()->RetConnect(c);
        return;
    }    
    if(outstr != "") {
        //clear connect
        //c->OutString(outstr);
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


 

//void GentEvent::HandleMain(struct evhttp_request *request, void *arg) {
//	cout<< "GentEvent::HandleMain push11111111" << endl;
//	GentEvent *gevent = static_cast<GentEvent *>(arg);
//	COMM_PACK pack;
//	pack.type_ = request->type;
//	pack.type_  = 12;
//	pack.request_ = request;
//	cout<< "GentEvent::HandleMain push" << endl;
////	if(GentFrame::Instance()->msg_.Cursize() == GentFrame::Instance()->msg_.Getsize()) {
////		GentThread::Intance()->SendThread();
////	}
//	cout << "start push" << endl;
//	GentFrame::Instance()->msg_.Push(pack);
//	cout << "end push" << endl;
//#ifdef DEBUG
//	//cout << request->uri << endl;
//#endif
//	pack.UrlParse(request->uri);
//	gevent->ContentParse(pack,request);
//
//
//
////	GentThread::Intance()->SendThread();
////	evhttp_connection_set_closecb(request->evcon, GentEvent::Close, NULL);
////	GentBasic *app = NULL;
////	if(GentFrame::Instance()->GetModule(app, 1)==-1) {
////		printf("GetModule failed\n");
////	}else{
////		GentRep gr;
////		app->http_event_ = &gr;
////		app->Proccess();
////	}
//
//
//	COMM_PACK pack2 = GentFrame::Instance()->msg_.Pop();
//	cout << "pack2.type:" << pack2.type_<<endl;
//
//	struct evbuffer *return_buffer=evbuffer_new();
//	char buf[10] = "ok";
//	evbuffer_add_printf(return_buffer,"%s",buf);
//	evhttp_send_reply(request,HTTP_OK,"Client",return_buffer);
//	evbuffer_free(return_buffer);
//
//}

void GentEvent::Close(struct evhttp_connection *http_conn, void *args) {
	//cout << "GentEvent::Close"<< endl;
}
//void GentEvent::ContentParse(COMM_PACK &pack, struct evhttp_request *&request) {
//	if(request->type != EVHTTP_REQ_POST) return;
//	cout << request->input_buffer->buffer << endl;
//	pack.ContentParse((char *)request->input_buffer->buffer);
//}
//
//GentRep *GentRep::intance_ = NULL;
//
//GentRep *GentRep::Intance() {
//	if(intance_ == NULL) {
//		intance_ = new GentRep();
//	}
//	return intance_;
//}
//
//void GentRep::UnIntance() {
//	if(intance_) delete intance_;
//}
//
//void GentRep::Ret(COMM_REP &rep) {
//	//if(rep_.Cursize()>0) {
//		//COMM_REP rep =rep_.Pop();
//		if(rep.request_) {
//			Response(rep.request_,rep.ret_buff_);
//		}
//	//}
//}
//void GentRep::Response(struct evhttp_request *request,char *buf) {
//	struct evbuffer *return_buffer=evbuffer_new();
//	//char buf[10]="ok";
//	evbuffer_add_printf(return_buffer,"%s",buf);
//	evhttp_send_reply(request,HTTP_OK,"Client",return_buffer);
//	evbuffer_free(return_buffer);
//}
/*
void GentEvent::HandleMain1(struct evhttp_request *request, void *arg) {
	printf("ddd\n");
	 struct sockaddr addr;
	 socklen_t addrlen = sizeof(addr);
	 int sfd;

	 if ((sfd = accept(fd, &addr, &addrlen)) == -1) {
		 printf("accept failed\n");
		 return;
	 }
	 char buf[2048];
	 read(sfd,buf,10);
	 printf("%s\n",buf);
	 GentBasic *app = NULL;
	 if(GentFrame::Instance()->GetModule(app, 1)==-1) {
		 printf("GetModule failed\n");
	 }else{
		 app->Proccess();
	 }
	 int flags = fcntl(sfd, F_GETFL);
	 fcntl(sfd, F_SETFL, flags | O_NONBLOCK);
	 send(sfd,"error",5,0);
	 close(sfd);
}
*/
