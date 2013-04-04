/*
 * gent_thread.h
 *
 *  Created on: 2012-3-25
 *      Author: wyt
 */

#ifndef GENT_THREAD_H_
#define GENT_THREAD_H_
#include <event.h>
class GentEvent;

#define MAX_THREAD	50
typedef struct THREADINFO {
	int rec_id;
	int send_id;
	int id;
	unsigned thread_id;
	void *th;
    GentEvent *gevent;
	//struct event event_;
	//struct event_base *evbase;
}THREADINFO;

class GentThread
{
	static GentThread *intanceth_;

	int thread_count_;
	THREADINFO threads_[MAX_THREAD];
	//struct event_base *base_;
	//struct event_base *evbase_;
	int lastid_;
public:
	static GentThread *Intance();
	static void UnIntance();
	static void* Work(void *);
	static void Handle(int fd, short which, void *arg);
public:
	GentThread();
	~GentThread();
	void Start();
	void init(int thread_count=1);
	void SendThread();
	void SetupThread(THREADINFO *thread);
	static void* Handle2(void *arg);

};

#endif /* GENT_THREAD_H_ */
