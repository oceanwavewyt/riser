/*
 * gent_app_mgr.h
 *
 *  Created on: 2012-7-7
 *      Author: wyt
 */
#include "gent_app_mgr.h"
#include "gentle.h"
#include "prefine.h"

GentAppMgr *GentAppMgr::intance_ = NULL;

GentAppMgr *GentAppMgr::Instance() {
	if(intance_ == NULL) {
		intance_ = new GentAppMgr();
	}
	return intance_;
}

void GentAppMgr::UnInstance() {
	if(intance_) delete intance_;
}


GentAppMgr::GentAppMgr():def_num(10)
{
    total_conn = 0;
}

GentAppMgr::~GentAppMgr()
{

}

int GentAppMgr::Register(int cmd, GentBasic *app)
{
	if(app_mgr_.count(cmd)) return 1;
	if(app_mgr_[cmd].size()>=def_num) return 1;
	vector<GentBasic *> module_vec;
	for(unsigned int i=0;i<def_num; i++) {
		GentBasic *p = app->CloneProccess();
		module_vec.push_back(p);
	}
	app_mgr_[cmd] = module_vec;
	delete app;
	return 0;
}
int GentAppMgr::GetModule(int cmd, GentBasic *&app)
{
	APP_MODULE::iterator iter = app_mgr_.find(cmd);
	if(iter == app_mgr_.end()){
		//写日志
		cout << "GentAppMgr::GetModule: no find " << cmd << endl;
		return 1;
	}
	size_t len = (*iter).second.size();
	if(len<=0){
		cout << "GentAppMgr::GetModule: len " << len << endl;
		return 1;
	}
	app = (*iter).second[len-1];
	(*iter).second.pop_back();
	return 0;
}

int GentAppMgr::SetModule(int cmd, GentBasic *&app)
{
	APP_MODULE::iterator iter = app_mgr_.find(cmd);
	if(iter == app_mgr_.end()){
			//写日志
		return 1;
	}
	(*iter).second.push_back(app);
	return 0;
}

void GentAppMgr::SetPlugin(GentCommand *command) 
{
	plus = command;	
}
       
bool GentAppMgr::Init()
{
    string msg;
    if(plus->Init(msg)) {
        return false;
    }
	return true;
}

GentConnect *GentAppMgr::GetConnect(int sfd)
{
    size_t len = conn_mgr.size();
    if(len > 0) {
        AutoLock lock(&conn_lock);
        GentConnect *c= conn_mgr[len-1];
        c->Init(sfd);
        conn_mgr.pop_back();
        return c;
    }
    GentConnect *c = new GentConnect(sfd);
    total_conn++;
    return c;
}

GentCommand *GentAppMgr::GetCommand(GentConnect *connect,int id)
{
	GentCommand *p = plus->Clone(connect);
	plus_mgr[id] = p;
	return p;		
}

void GentAppMgr::Destroy(int id)
{
	PLUGIN::iterator iter = plus_mgr.find(id);
	if(iter != plus_mgr.end()){
		delete plus_mgr[id];	
	}
}

void GentAppMgr::RetConnect(GentConnect *c)
{
    assert(c!=NULL);
    AutoLock lock(&conn_lock);
    conn_mgr.push_back(c);
}

size_t GentAppMgr::GetConnCount()
{
    return total_conn - conn_mgr.size();
}

size_t GentAppMgr::GetTotalConnCount()
{
    return total_conn;
}
