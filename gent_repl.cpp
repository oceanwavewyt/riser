#include "gent_repl.h"
#include "gent_db.h"
#include "gent_event.h"
#include "gent_app_mgr.h"
#include "gent_frame.h"

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


GentRepMgr::GentRepMgr():connect_(NULL)
{
}

GentRepMgr::~GentRepMgr()
{
	std::map<string,GentReplication*>::iterator it;
	for(it=rep_list_.begin();it!=rep_list_.end();it++){
		delete (*it).second;
	}
}

void GentRepMgr::Handle(int fd, short which, void *arg) 
{
	GentEvent *e = static_cast<GentEvent *>(arg);
	cout << "create slave thread" <<endl;
	GentConfig &config = GentFrame::Instance()->config;
	GentRepMgr::Instance()->Slave(config["slavename"], e);	
}

void GentRepMgr::Slave(const string &client_name, GentEvent *ev_)
{
	if(connect_)  return ;
	cout << "uesss"<<endl;
	int sfd = ev_->Client("127.0.0.1",3555);		
	if(sfd < 0) return;
	char str[300] = {0};
	snprintf(str, 300,"*3\r\n$3\r\nrep\r\n$%ld\r\n%s\r\n$4\r\nauth\r\n$6\r\n123456\r\n",
			client_name.size(),client_name.c_str());
	int slen = send(sfd, str, 300, 0);             
	if (slen == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {            
    	return;                                                   
	}                                                             			
	if(slen < 0) return;	
	connect_ = GentAppMgr::Instance()->GetConnect(sfd);
	connect_->is_slave = true;
	connect_->gevent = ev_;
	ev_->AddEvent(connect_, GentEvent::Handle);		
}

void GentRepMgr::SlaveReply(string &outstr, int suc)
{
	string t = (suc==1)?"ok":"error";
	char str[300] = {0};
	GentConfig &config = GentFrame::Instance()->config;	
	snprintf(str, 300,"*3\r\n$3\r\nrep\r\n$%ld\r\n%s\r\n$%ld\r\n%s\r\n",
			config["slavename"].size(),config["slavename"].c_str(), t.size(),t.c_str());
	outstr = str;
}

void GentRepMgr::CannelConnect() {
	connect_ = NULL;
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
	if(it == rep_list_.end()) {
		rep_list_[name] = new GentReplication(name);
	}	
	return rep_list_[name];
}

bool GentRepMgr::Run(string &name,string &msg, string &outstr)
{
	GentReplication *rep = Get(name);	
	rep->Start(msg, outstr);		
	return true;
}

void GentRepMgr::Push(int type, string &key)
{
	std::map<string,GentReplication*>::iterator it;
	for(it=rep_list_.begin(); it!=rep_list_.end(); it++)
	{
		it->second->Push(type, key);
	}
}

GentReplication::GentReplication(const string &name):status(0)
{
	cout <<"init GentReplication" <<endl;
	rep_name = name;
	current_node = NULL;
	//设置所有的磁盘数据到队列
	is_update_main_que = true;	
	vector<string> outvec;
	vector<string>::iterator it;
	GentDb::Instance()->Keys(outvec, "*");
	for(it=outvec.begin();it!=outvec.end();it++) {
		itemData *item = new itemData(*it,itemData::ADD);
		main_que.push(item);
	}
	is_update_main_que = false;	
}

GentReplication::~GentReplication(){

}

void GentReplication::Push(int type, string &key)
{

}

bool GentReplication::Start(string &msg, string &outstr)
{
	if(status == 1) {
		status = 0;
		//删除节点数据
		if(msg == "ok") {
			main_que.pop();	
		}else if(msg == "error") {
			outstr = "*2\r\n$3\r\nrep\r\n$5\r\nclose\r\n";
			return false;
		}
	}
	//同步数据
	itemData *it = main_que.front_element();
	if(it == NULL) return false;
	status = 1;
	if(it->type == itemData::ADD) {
		string nr = "";
		cout << "itemData: "<< it->name << endl;
		if(!GentDb::Instance()->Get(it->name, nr)) {
			Reply(itemData::DEL, it->name,	outstr);
			return true;		
		}
		Reply(itemData::ADD, it->name, outstr, nr);
		return true;
	}				
	Reply(itemData::DEL, it->name, outstr);
	return true;

}

void GentReplication::Reply(int type, string &key,string &outstr, const string &nr)
{
	char retstr[300] = {0};
	if(type == itemData::ADD) {
		snprintf(retstr,300,"*3\r\n$3\r\nset\r\n$%ld\r\n%s\r\n$%ld\r\n",
				key.size(),key.c_str(),nr.size());
		outstr=retstr+nr+"\r\n";
	}else if(type == itemData::DEL){
		snprintf(retstr,300,"*2\r\n$3\r\ndel\r\n$%ld\r\n%s\r\n",key.size(), key.c_str());
		outstr=retstr;		
	}	
}



