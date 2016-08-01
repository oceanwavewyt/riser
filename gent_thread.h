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
    	//GentEvent *gevent;
	struct event event;
	struct event_base *base;
}THREADINFO;

class GentThread
{
	static GentThread *intanceth_;

	int thread_count_;
	THREADINFO *threads_;
	THREADINFO *master_thread_;
	//struct event_base *base_;
	//struct event_base *evbase_;
	int lastid_;
public:
	static GentThread *Intance();
	static void UnIntance();
	static void* Work(void *);
	static void *Rep(void *);
	static void *Master(void *);
	static void Handle(int fd, short which, void *arg);
public:
	GentThread();
	~GentThread();
	void Start();
	void init(int fd, int thread_count=1);
	bool SendThread(dataItem *d);
	bool SendMasterMsg(GentReplication *r);
	void SetupThread(THREADINFO *thread, void(*handle)(const int fd, const short which, void *arg));
	static void* ClearHandle(void *arg);
	bool StartClear();
};

#endif /* GENT_THREAD_H_ */
