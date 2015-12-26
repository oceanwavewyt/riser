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
typedef std::vector<GentConnect *> FREE_CONNPOOL;
typedef std::map<int, GentConnect *> CONNPOOL;



class GentAppMgr
{
	static GentAppMgr *intance_;
	static  CommLock lock;
private:
	APP_MODULE app_mgr_;
	PLUGIN plus_mgr;
	unsigned int def_num;
	GentCommand *plus;
    CONNPOOL conn_mgr;
	FREE_CONNPOOL free_conn_mgr;
    CommLock conn_lock;
    size_t total_conn;
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
    size_t GetTotalConnCount();
	bool Init();
	void SetPlugin(GentCommand *command);	
	GentCommand *GetCommand(GentConnect *, int id);
	void Destroy(int id);		
};


#endif /* GENT_APP_MGR_H_ */
