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
CommLock GentAppMgr::lock;

GentAppMgr *GentAppMgr::Instance() {
	AutoLock loc(&lock);
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
/*
GentConnect *GentAppMgr::GetConnect(int sfd)
{
    size_t len = free_conn_mgr.size();
    AutoLock lock(&conn_lock);
	GentConnect *c;
    if(len > 0) {
        c= free_conn_mgr[len-1];
        c->Init(sfd);
        free_conn_mgr.pop_back();
    }else{
    	 c = new GentConnect(sfd);
         c->Init(sfd);
    	total_conn++;
    }
    LOG(GentLog::BUG,"add fd:%d to conn_mgr", sfd); 
    conn_mgr[sfd] = c;
    return c;
}
*/
GentConnect *GentAppMgr::GetConnect(int sfd)
{
    AutoLock lock(&conn_lock);
    size_t len = free_conn_mgr.size();
	GentConnect *c;
    if(len > 0) {
        c= free_conn_mgr.back();
        c->Init(sfd);
        free_conn_mgr.pop_back();
    }else{
    	 c = new GentConnect(sfd);
         c->Init(sfd);
    	 total_conn++;
    }
    LOG(GentLog::BUG,"add fd:%d to conn_mgr", sfd); 
    conn_mgr[sfd] = c;
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
		if(plus_mgr[id]) 
			delete plus_mgr[id];	
	}
}

void GentAppMgr::RetConnect(GentConnect *c)
{
    AutoLock lock(&conn_lock);
    if(!c) return;
	CONNPOOL::iterator it = conn_mgr.find(c->fd);
	if(it == conn_mgr.end()) {
		LOG(GentLog::FATAL,"link fd:%d not find", c->fd);		
		exit(1);
	}
	c->fd = -1;
	free_conn_mgr.push_front(c);
	conn_mgr.erase(it);
	//check link num
	size_t linkNum = GetConnCount()*20;
	if(linkNum < free_conn_mgr.size()) {
		size_t num = free_conn_mgr.size() - linkNum;
		for(size_t i=1; i<=num; i++) {
			GentConnect *conn = free_conn_mgr.front();
			delete conn;
			total_conn--;
			free_conn_mgr.pop_front();
		}
	}
}

size_t GentAppMgr::GetConnCount()
{
    return total_conn - free_conn_mgr.size();
}

size_t GentAppMgr::GetTotalConnCount()
{
    return total_conn;
}
