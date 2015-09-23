/*
 * gent_event.h
 *
 *  Created on: 2012-2-19
 *      Author: wyt
 */

#ifndef GENT_EVENT_H_
#define GENT_EVENT_H_
#include <event.h>
#include <evhttp.h>
#include "gent_msg.h"
#include "gent_connect.h"

class GentFrame;

//typedef void (*handler)(const int fd, const short which, void *arg);
static const int eventRead = EV_READ | EV_PERSIST;
static const int eventWrite = EV_WRITE | EV_PERSIST;

class GentEvent
{
	static GentEvent *intance_;
public:
	struct event ev_;
	struct event_base *main_base_;
	string ip_;
	unsigned int port_;
public:
	GentEvent();
	~GentEvent();
	int AddEvent(GentConnect *,void (*)(const int, const short, void *));
    void DelEvent() {event_del(&ev_);};
	int UpdateEvent(int fd,GentConnect *c, int);
	int AddTimeEvent(struct timeval *tv, void (*)(const int, const short, void *));
	void Loop();
	int AcceptSocket(struct evhttp *http, int fd);
	int Client(const string &host, int port);
//	void ContentParse(COMM_PACK &pack,struct evhttp_request *&request);
	static void HandleMain(const int fd, const short which, void *arg);
	static void Handle(const int fd, const short which, void *arg);
	static void Close(struct evhttp_connection *http_conn, void *args);
public:
	static GentEvent *Instance();
	static void UnIntance();
};
/*
class GentRep {
	static GentRep *intance_;
public:
	GENT_REP_COMM rep_;
public:
	GentRep(){
		rep_.Resize(10000);
	}
	~GentRep(){
		if(GentRep::intance_) {
			delete	GentRep::intance_;
		}
	}
	void SetRet(string &buf);
	void Ret(COMM_REP &rep);
	void Response(struct evhttp_request *request,char *buf);
public:
	static GentRep *Intance();
	static void UnIntance();
};
*/
#endif /* GENT_EVENT_H_ */
