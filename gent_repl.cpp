#include "gent_repl.h"
#include "gent_db.h"
#include "gent_event.h"
#include "gent_app_mgr.h"
#include "gent_frame.h"

GentRepMgr *GentRepMgr::intanceMaster_ = NULL;
GentRepMgr *GentRepMgr::intanceSlave_ = NULL;

GentRepMgr *GentRepMgr::Instance(const string &name) {
	if(name == "master") {
		if(intanceMaster_ == NULL) {
			intanceMaster_ = new GentRepMgr();
		}
		return intanceMaster_;		
	}
	if(intanceSlave_ == NULL) {
		intanceSlave_ = new GentRepMgr();
	}
	return intanceSlave_;
}

void GentRepMgr::UnInstance() {
	if(intanceMaster_) delete intanceMaster_;
	if(intanceSlave_) delete intanceSlave_;
}


GentRepMgr::GentRepMgr():connect_(NULL),status(GentRepMgr::INIT)
{
}

GentRepMgr::~GentRepMgr()
{
	std::map<string,GentReplication*>::iterator it;
	for(it=rep_list_.begin();it!=rep_list_.end();it++){
		delete (*it).second;
	}
}

void GentRepMgr::SlaveHandle(int fd, short which, void *arg) 
{
	GentEvent *e = static_cast<GentEvent *>(arg);
	cout << "create slave thread" <<endl;
	GentRepMgr::Instance("slave")->Slave(e);
}

void GentRepMgr::Slave(GentEvent *e) 
{
	if(status == GentRepMgr::AUTH || status == GentRepMgr::TRAN) return;
	GentConfig &config = GentFrame::Instance()->config;
	if(!connect_) {
		if(config["slaveof_ip"] == "" || config["slaveof_port"] == "") return;
		int port = atoi(config["slaveof_port"].c_str());
		if(LinkMaster(e, config["slaveof_ip"], port)<=0) return;
		status = GentRepMgr::INIT;
	}
	if(status == GentRepMgr::INIT) {
		//认证过程	
		int sdnum = SlaveAuth(e, config["slavename"]);
		if(sdnum == -1) {
			CannelConnect();
			return;
		}
		status = GentRepMgr::AUTH;
		return;
	}else if(status == GentRepMgr::COMPLETE) {
		char str[300] = {0};
		snprintf(str, 300,"*3\r\n$3\r\nrep\r\n$%ld\r\n%s\r\n$4\r\ndata\r\n",
				config["slavename"].size(),config["slavename"].c_str());
		string sendstr(str);
		int sdnum = connect_->OutString(sendstr);
		if(sdnum == -1) {		
			CannelConnect();
			return;
		}
		status = GentRepMgr::TRAN;
	}
}

int GentRepMgr::LinkMaster(GentEvent *ev_, const string &host, int port) {
	int sfd = ev_->Client(host,port);		
	if(sfd < 0) return sfd;
	connect_ = GentAppMgr::Instance()->GetConnect(sfd);
	connect_->is_slave = true;
	connect_->gevent = ev_;
	return sfd;
}

int GentRepMgr::SlaveAuth(GentEvent *ev_, const string &client_name)
{
	char str[300] = {0};
	snprintf(str, 300,"*4\r\n$3\r\nrep\r\n$%ld\r\n%s\r\n$4\r\nauth\r\n$6\r\n123456\r\n",
			client_name.size(),client_name.c_str());
	string sendstr(str);
	int ret = connect_->OutString(sendstr);
	if(ret >= 0) {
		ev_->AddEvent(connect_, GentEvent::Handle);
	}
	return ret;
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

void GentRepMgr::SlaveSetStatus(int t) 
{
	status = t;	
}

void GentRepMgr::CannelConnect() {
	event_del(&connect_->ev);                    
	if(connect_->fd > 0) {
		connect_->Destruct();                        
		GentAppMgr::Instance()->RetConnect(connect_);
	}
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
	itemData *item = new itemData(key,type);
	main_que.push(item);
}

bool GentReplication::Start(string &msg, string &outstr)
{
	if(msg == "auth" ) {
		outstr = "*2\r\n$5\r\nreply\r\n$6\r\nauthok\r\n";
		return true;
	}
	if(msg != "ok" && msg != "data"){
		 outstr = "*2\r\n$5\r\nreply\r\n$5\r\nclose\r\n";
		 return false;	
	}
	if(status == 1) {
		status = 0;
		//删除节点数据
		if(msg == "ok") {
			itemData *it = main_que.pop();
			delete it;
			it = NULL;	
		}else if(msg == "error") {
			outstr = "*2\r\n$5\r\nreply\r\n$5\r\nclose\r\n";
			return false;
		}
	}
	//同步数据
	itemData *it = main_que.front_element();
	if(it == NULL) {
		outstr = "*2\r\n$5\r\nreply\r\n$8\r\ncomplete\r\n";
		return false;
	}
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



