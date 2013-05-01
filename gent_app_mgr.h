/*
 * gent_app_mgr.h
 *
 *  Created on: 2012-7-7
 *      Author: wyt
 */

#ifndef GENT_APP_MGR_H_
#define GENT_APP_MGR_H_
#include "prefine.h"
class GentBasic;
class GentCommand;
class GentConnect;
typedef std::map<int,std::vector<GentBasic *> > APP_MODULE;
typedef std::map<int,GentCommand*> PLUGIN;
typedef std::vector<GentConnect *> CONNPOOL;


class CommLock
{
private:
    pthread_mutex_t _lock;
public:
    CommLock()
	{
		pthread_mutex_init(&_lock,NULL);
	}
	~CommLock(){}
	void Lock()
	{
		pthread_mutex_lock(&_lock);
	}
	void UnLock()
	{
		pthread_mutex_unlock(&_lock);
	}
};

class AutoLock{
	CommLock* _lock;
public:
	AutoLock(CommLock * lock)
	{
		_lock = lock;
		_lock->Lock();
	}
    
    ~AutoLock()
	{
		_lock->UnLock();
	}
};

class GentAppMgr
{
	static GentAppMgr *intance_;
private:
	APP_MODULE app_mgr_;
	PLUGIN plus_mgr;
	unsigned int def_num;
	GentCommand *plus;
    CONNPOOL conn_mgr;
    CommLock conn_lock;
public:
	static GentAppMgr *Instance();
	static void UnInstance();
public:
	GentAppMgr();
	~GentAppMgr();
	int Register(int cmd, GentBasic *app);
	int GetModule(int cmd,GentBasic *&);
	int SetModule(int cmd, GentBasic *&app);
	GentConnect *GetConnect(int sfd);
    void RetConnect(GentConnect *);
    size_t GetConnCount();
	bool Init();
	void SetPlugin(GentCommand *command);	
	GentCommand *GetCommand(GentConnect *, int id);
	void Destroy(int id);		
};


#endif /* GENT_APP_MGR_H_ */
