#include "gent_repl.h"
#include "gent_db.h"

GentRepMgr *GentRepMgr::intance_ = NULL;

GentRepMgr *GentRepMgr::Instance() {
	if(intance_ == NULL) {
		intance_ = new GentRepMgr();
	}
	return intance_;
}

void GentRepMgr::UnInstance() {
	if(intance_) delete intance_;
}


GentRepMgr::GentRepMgr()
{
}

GentRepMgr::~GentRepMgr()
{
	std::map<string,GentReplication*>::iterator it;
	for(it=rep_list_.begin();it!=rep_list_.end();it++){
		delete (*it).second;
	}
}


bool GentRepMgr::Logout(string &name)
{
	//注销队列
		
	return true;
}

GentReplication *GentRepMgr::Get(string &name)
{
	std::map<string,GentReplication*>::iterator it;
	it = rep_list_.find(name); 	
	if(it != rep_list_.end()) {
		rep_list_[name] = new GentReplication(name);
	}	
	return rep_list_[name];
}

bool GentRepMgr::Run(string &name, string &runid)
{
	GentReplication *rep = Get(name);	
	rep->Start();		
	return true;
}

GentReplication::GentReplication(const string &name):status(0)
{
	rep_name = name;
	current_node = NULL;
	//设置所有的磁盘数据到队列
	is_update_main_que = true;	
	vector<string> outvec;
	vector<string>::iterator it;
	GentDb::Instance()->Keys(outvec, "*");
	for(it=outvec.begin();it!=outvec.end();it++) {
		itemData item(*it,2);
		main_que.push(&item);
	}
	is_update_main_que = false;	
}

GentReplication::~GentReplication(){

}

bool GentReplication::Check()
{

}

bool GentReplication::Start()
{
		


	if(status == 0) {
		//同步数据
		

	}else if(status == 1) {
		//删除节点数据
	
	}	
	return true;

}


