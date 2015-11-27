#include "gent_repl.h"
#include "gent_db.h"
#include "gent_event.h"
#include "gent_app_mgr.h"
#include "gent_frame.h"
#include "gent_util.h"

GentRepMgr *GentRepMgr::intanceMaster_ = NULL;
GentRepMgr *GentRepMgr::intanceSlave_ = NULL;

GentRepMgr *GentRepMgr::Instance(const string &name) {
	if(name == "master") {
		if(intanceMaster_ == NULL) {
			intanceMaster_ = new GentRepMgr(name);
		}
		return intanceMaster_;		
	}
	if(intanceSlave_ == NULL) {
		intanceSlave_ = new GentRepMgr(name);	
	}
	return intanceSlave_;
}

void GentRepMgr::UnInstance() {
	if(intanceMaster_) delete intanceMaster_;
	if(intanceSlave_) delete intanceSlave_;
}


GentRepMgr::GentRepMgr(const string &name):connect_(NULL),status(GentRepMgr::INIT)
{
	if(name == "master") {
   		string pathname = GentDb::Instance()->GetPath()+"/REPLICATION_INFO";
   		repfile_ = new GentFile<repinfo>(pathname, SLAVE_NUM);	
		if(!repfile_->Init(rep_map_)) {
			LOG(GentLog::ERROR, "init replication infomation failed.");
		}
	}
	server_id_ = GentFrame::Instance()->s->server_id;
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
	//cout << "create slave thread" <<endl;
	GentRepMgr::Instance("slave")->Slave(e);
	e->DelEvent();
	struct timeval tv = {1,0};                    
	e->AddTimeEvent(&tv, GentRepMgr::SlaveHandle);
}

void GentRepMgr::Slave(GentEvent *e) 
{
	if(status == GentRepMgr::AUTH || status == GentRepMgr::WAIT){
		if(connect_ && connect_->fd>0) return;
		status = GentRepMgr::CONTINUE;
	}
	GentConfig &config = GentFrame::Instance()->config;
	if(!connect_) {
		if(config["slaveof_ip"] == "" || config["slaveof_port"] == "") return;
		int port = atoi(config["slaveof_port"].c_str());
		if(LinkMaster(e, config["slaveof_ip"], port)<=0) return;
		if(slave_start_time == 0) {
			slave_start_time = time(NULL);
		}
		status = GentRepMgr::INIT;
	}
	if(status == GentRepMgr::INIT) {
		if(config["master_auth"] == "") {
			status = GentRepMgr::CONTINUE;
			cout << "master_auth is null "<<endl;
		}else{
			//认证过程	
			int sdnum = SlaveAuth(server_id_, config["master_auth"]);
			if(sdnum == -1) {
				CannelConnect();
				return;
			}
			status = GentRepMgr::AUTH;
		}
		e->AddEvent(connect_, GentEvent::Handle);
		return;
	}else if(status == GentRepMgr::CONTINUE) {
		char str[300] = {0};
		snprintf(str, 300,"*3\r\n$3\r\nrep\r\n$%ld\r\n%s\r\n$4\r\ndata\r\n",
			(unsigned long)server_id_.size(),server_id_.c_str());
		string sendstr(str);
		int sdnum = connect_->OutString(sendstr);
		if(sdnum == -1) {		
			CannelConnect();
			return;
		}
		status = GentRepMgr::WAIT;
	}
}

int GentRepMgr::LinkMaster(GentEvent *ev_, const string &host, int port) {
	int sfd = ev_->Client(host,port);		
	if(sfd < 0) return sfd;
	connect_ = GentAppMgr::Instance()->GetConnect(sfd);
	connect_->SetAuth(1);
	connect_->is_slave = true;
	connect_->gevent = ev_;
	return sfd;
}

int GentRepMgr::SlaveAuth(const string &client_name, const string &auth)
{
	char str[300] = {0};
	snprintf(str, 300,"*4\r\n$3\r\nrep\r\n$%ld\r\n%s\r\n$4\r\nauth\r\n$%ld\r\n%s\r\n",
			(unsigned long)client_name.size(),client_name.c_str(), (unsigned long)auth.size(),auth.c_str());
	string sendstr(str);
	return connect_->OutString(sendstr);
}

void GentRepMgr::SlaveReply(string &outstr, int suc)
{
	string t = (suc==1)?"ok":"error";
	char str[300] = {0};
	snprintf(str, 300,"*3\r\n$3\r\nrep\r\n$%ld\r\n%s\r\n$%ld\r\n%s\r\n",
			(unsigned long)server_id_.size(),server_id_.c_str(), (unsigned long)t.size(),t.c_str());
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

GentReplication *GentRepMgr::Get(const string &name)
{
	
	std::map<string,GentReplication*>::iterator it = rep_list_.find(name); 	
	if(it == rep_list_.end()) {
		map<string,repinfo *>::iterator it_info = rep_map_.find(name);
		repinfo *info;
		if(it_info == rep_map_.end()) {
			info = repfile_->AddItem(name);
		}else{
			info = it_info->second;
		}
		if(info == NULL) return NULL;
		rep_list_[name] = new GentReplication(name, info);
	}	
	return rep_list_[name];
}

void GentRepMgr::Push(int type, string &key)
{
	std::map<string,GentReplication*>::iterator it;
	for(it=rep_list_.begin(); it!=rep_list_.end(); it++)
	{
		it->second->Push(type, key);
	}
}

void GentRepMgr::GetSlaveInfo(string &str)
{
	str = "";
	std::map<string,GentReplication*>::iterator it;
	for(it=rep_list_.begin(); it!=rep_list_.end(); it++)
	{
		string info;
		it->second->GetInfo(info);
		str+=info;
	}
	if(str == ""){
		str = "no slave\r\n";
	}
}

void GentRepMgr::GetInfo(string &str)
{
	GentConfig &config = GentFrame::Instance()->config;                       
    if(config["slaveof_ip"] != "" && config["slaveof_port"] != "")
	{
		char buf[200] = {0};
		map<int, string> st_map;
		st_map[GentRepMgr::INIT] = "initialization";
		st_map[GentRepMgr::AUTH] = "authentication";
		st_map[GentRepMgr::WAIT] = "wait";
		st_map[GentRepMgr::CONTINUE] = "transmit";
		snprintf(buf,200, "connect master %s:%s,start_time:%ld,stage: %s\r\n",
				config["slaveof_ip"].c_str(),config["slaveof_port"].c_str(),
				(unsigned long)slave_start_time,st_map[status].c_str());
		str = buf;
	}else{
		str = "no master\r\n";
	}
}

uint32_t GentRepMgr::GetReplicationNum()
{
	return rep_list_.size();
}

uint64_t GentRepMgr::QueLength()
{
	uint64_t len = 0;
	std::map<string,GentReplication*>::iterator it;
	for(it=rep_list_.begin(); it!=rep_list_.end(); it++)
	{
		len += it->second->QueLength();
	}
	return len;
}

GentReplication::GentReplication(const string &name, repinfo *rinfo):status(0),
main_que_length(0)
{
	slave_start_time = time(NULL);	
	rep_name = name;
	rinfo_ = rinfo;
	current_node = NULL;
	cout << "ser_time:"<<rinfo_->ser_time << endl;
	cout << "rep_time:"<<rinfo_->rep_time << endl;
	if(!rinfo_->ser_time || rinfo_->ser_time > rinfo_->rep_time) {
		LOG(GentLog::WARN, "sync all data for %s slave.", name.c_str());
		//设置所有的磁盘数据到队列
		is_update_que = true;	
		vector<string> outvec;
		vector<string>::iterator it;
		GentDb::Instance()->Keys(outvec, "*");
		for(it=outvec.begin();it!=outvec.end();it++) {
			itemData *item = new itemData(*it,itemData::ADD);
			que.push(item);
			main_que_length++;	
		}
		if(!rinfo_->ser_time) {
			AutoLock lock(&que_push_lock);
			rinfo_->set("ser_time",time(NULL));	
		}
	}else{
		LOG(GentLog::WARN, "not need to sync all data for %s slave.", name.c_str());
	}
}

GentReplication::~GentReplication(){

}

void GentReplication::Push(int type, string &key)
{
	AutoLock lock(&que_push_lock);
	itemData *item = new itemData(key,type);
	main_que.push(item);
	//通知slave
	if(main_que_length == 0) {
		if(conn_ && conn_->fd>0) {
			string outstr = "*2\r\n$5\r\nreply\r\n$8\r\ncomplete\r\n";
			conn_->SetWrite(outstr);	
		}
	}
	main_que_length++;
	rinfo_->set("ser_time",time(NULL));
}

void GentReplication::Pop()
{
	AutoLock lock(&que_push_lock);
	itemData *it;
	if(!is_update_que) {
	 	it= main_que.pop();
		rinfo_->set("rep_time",time(NULL));
	}else{
		it= que.pop();
		if(it == NULL && !rinfo_->rep_time ) {
			rinfo_->set("rep_time", time(NULL));
		}
	}
	if(it != NULL) {
		main_que_length--;
		delete it;
	}
	it = NULL;
}

itemData *GentReplication::front_element()
{
	AutoLock lock(&que_push_lock);
	if(!is_update_que) {
	 	return main_que.front_element();
	}
	itemData *it= que.front_element();
	if(it == NULL) {
		is_update_que = false;
		if(!rinfo_->rep_time ) {
			rinfo_->set("rep_time", time(NULL));	
		}
		return main_que.front_element();
	}
	return it;
}

void GentReplication::GetInfo(string &str)
{
	char ret[500] = {0};
	string s = "";
	if(!conn_) {
		s = "close";
	}else{
		s = conn_->GetStatus();
	}
	snprintf(ret,500,"name:%s,ip:%s,start:%s,last:%s,status:%s,need_sync: %lld\r\n",
			rep_name.c_str(),slave_ip.c_str(),
			GentUtil::TimeToStr(slave_start_time).c_str(),
			GentUtil::TimeToStr(slave_last_time).c_str(),
			s.c_str(),
			(unsigned long long)main_que_length);
	str = ret;
}

bool GentReplication::Start(string &msg, GentConnect *c, string &outstr)
{
	conn_ = c;
	slave_ip = conn_->ip;
	c->RegDestroy(rep_name, this);
	slave_last_time = time(NULL);
	if(msg == "auth" ) {
		outstr = "*2\r\n$5\r\nreply\r\n$6\r\nauthok\r\n";
		return true;
	}else if(msg == "autherror"){
		outstr = "*2\r\n$5\r\nreply\r\n$9\r\nautherror\r\n";
		return true;
	}
	if(msg != "ok" && msg != "data"){
		 outstr = "*2\r\n$5\r\nreply\r\n$5\r\nclose\r\n";
		 return true;	
	}
	if(status == 1) {
		status = 0;
		//删除节点数据
		if(msg == "ok") {
			Pop();	
		}else if(msg == "error") {
			outstr = "*2\r\n$5\r\nreply\r\n$5\r\nclose\r\n";
			return true;
		}
	}
	//同步数据
	itemData *it = front_element();
	if(it == NULL) {
		//outstr = "*2\r\n$5\r\nreply\r\n$8\r\ncomplete\r\n";
		outstr = "";
		return false;
	}
	status = 1;
	if(it->type == itemData::ADD) {
		string nr = "";
		LOG(GentLog::INFO, "sync the key of %s data.", it->name.c_str());
		uint64_t expire;
		if(!GentDb::Instance()->Get(it->name, nr, expire)) {
			Reply(itemData::DEL, it->name,	outstr);
			return true;		
		}
		uint64_t t = (unsigned long long)time(NULL);
		if(expire != 0 && expire < t) {
			Reply(itemData::DEL, it->name,  outstr);
			return true;
		}
		if(expire != 0) {
			expire = expire - t;
		}
		LOG(GentLog::INFO, "reply %s.", it->name.c_str());
		Reply(itemData::ADD, it->name, outstr, nr, expire);
		return true;
	}				
	Reply(itemData::DEL, it->name, outstr);
	return true;

}

void GentReplication::Reply(int type, string &key,string &outstr, const string &nr, uint64_t expire)
{
	char retstr[300] = {0};
	if(type == itemData::ADD) {
		if(expire == 0){
			snprintf(retstr,300,"*3\r\n$3\r\nset\r\n$%ld\r\n%s\r\n$%ld\r\n",
				(unsigned long)key.size(),key.c_str(),(unsigned long)nr.size());
		}else{
			char expstr[20]={0};
			snprintf(expstr, 20, "%llu",(unsigned long long)expire);
			snprintf(retstr,300,"*4\r\n$5\r\nsetex\r\n$%ld\r\n%s\r\n$%ld\r\n%s\r\n$%ld\r\n",
				(unsigned long)key.size(),key.c_str(),(unsigned long)strlen(expstr),expstr, (unsigned long)nr.size());
		}
		outstr=retstr+nr+"\r\n";
	}else if(type == itemData::DEL){
		snprintf(retstr,300,"*2\r\n$3\r\ndel\r\n$%ld\r\n%s\r\n",(unsigned long)key.size(), key.c_str());
		outstr=retstr;		
	}
	//cout << outstr <<endl;	
}



