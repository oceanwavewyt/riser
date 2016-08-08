#include "gent_repl.h"
#include "gent_db.h"
#include "gent_event.h"
#include "gent_app_mgr.h"
#include "gent_frame.h"
#include "gent_util.h"
#include "gent_thread.h"

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
	slave_send_time = 0;
	slave_start_time = 0;
}

GentRepMgr::~GentRepMgr()
{
	std::map<string,GentReplication*>::iterator it;
	for(it=rep_list_.begin();it!=rep_list_.end();it++){
		delete (*it).second;
	}
}

void GentRepMgr::MasterHandle(int fd, short which, void *arg)
{
	char buf[2];
	 if (read(fd, buf, 1) != 1){
         LOG(GentLog::WARN, "Can't read from libevent pipe");
	 }
     GentReplication *rep = GentFrame::Instance()->master_msg_.Pop();
	 if(!rep->GetConn()) return;
	 GentRedis *redis =static_cast<GentRedis *>(rep->GetConn()->comm);
	 rep = GentRepMgr::Instance("master")->Get(redis->keystr, redis->is_sync_all); 
	 string outstr;
	 if(rep == NULL) {
		outstr = REDIS_ERROR+" slave full,manual clear\r\n";
	 }else{
	 	if(!rep->Start(redis->repmsg, redis->conn, outstr)) {
			redis->conn->SetStatus(Status::CONN_WAIT);
	 	}
	}
	redis->conn->OutString(outstr);
	redis->conn->Reset();	
}

void GentRepMgr::SlaveHandle(int fd, short which, void *arg) 
{
	GentEvent *e = static_cast<GentEvent *>(arg);
	GentRepMgr::Instance("slave")->Slave(e);
	e->DelEvent();
	struct timeval tv = {1,0};                    
	int ret = e->AddTimeEvent(&tv, GentRepMgr::SlaveHandle);
	while(ret == -1) {
    	     ret = e->AddTimeEvent(&tv, GentRepMgr::SlaveHandle);
	}                                                       
}

void GentRepMgr::Slave(GentEvent *e) 
{
	//string s = (conn_)?conn_->GetStatus():"null";	
	//LOG(GentLog::WARN, "GentRepMgr::Slave,status: %d,conn:%s",status, s.c_str());
	if(status == GentRepMgr::AUTH || status == GentRepMgr::WAIT){
		if(conn_ && conn_->fd>0){
			if(slave_send_time > 0 && time(NULL) - slave_send_time > 60) {
				//CannelConnect();
				status = GentRepMgr::CONTINUE;
			}
			return;
		}
		status = GentRepMgr::CONTINUE;
	}
	GentConfig &config = GentFrame::Instance()->config;
	if(!conn_ || conn_->GetStatus() == "close") {
		if(config["slaveof_ip"] == "" || config["slaveof_port"] == "") return;
		int port = atoi(config["slaveof_port"].c_str());
		if(conn_) {
			conn_ = NULL;
			delete conn_;
		}
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
				//CannelConnect();
				return;
			}
			status = GentRepMgr::AUTH;
		}
		e->AddEvent(conn_, GentEvent::Handle);
		return;
	}else if(status == GentRepMgr::CONTINUE) {
		char str[300] = {0};
		snprintf(str, 300,"*3\r\n$3\r\nrep\r\n$%ld\r\n%s\r\n$4\r\ndata\r\n",
			(unsigned long)server_id_.size(),server_id_.c_str());
		string sendstr(str);
		int sdnum = conn_->OutString(sendstr);
		if(sdnum == -1) {		
			//CannelConnect();
			return;
		}
		slave_send_time = time(NULL);
		status = GentRepMgr::WAIT;
	}
}

int GentRepMgr::LinkMaster(GentEvent *ev_, const string &host, int port) {
	int sfd = ev_->Client(host,port);		
	if(sfd < 0) return sfd;
	conn_ = GentAppMgr::Instance()->GetConnect(sfd);
	conn_->RegDestroy(this);
	conn_->SetAuth(1);
	conn_->is_slave = true;
	return sfd;
}

int GentRepMgr::SlaveAuth(const string &client_name, const string &auth)
{
	char str[300] = {0};
	GentConfig &config = GentFrame::Instance()->config;
	string is_sync_all = "true";
	if(config["slave_sync_all"] == "false") {
		is_sync_all = "false";
	} 
	snprintf(str, 300,"*4\r\n$3\r\nrep\r\n$%ld\r\n%s\r\n$4\r\nauth\r\n$%ld\r\n%s\r\n$%ld\r\n%s\r\n",
			(unsigned long)client_name.size(),client_name.c_str(), 
			(unsigned long)auth.size(),auth.c_str(),
			(unsigned long)is_sync_all.size(),is_sync_all.c_str());
	string sendstr(str);
	return conn_->OutString(sendstr);
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
	event_del(&conn_->ev);                    
	if(conn_->fd > 0) {
		conn_->Destruct();                        
		GentAppMgr::Instance()->RetConnect(conn_);
	}
	conn_ = NULL;
}
bool GentRepMgr::Logout(string &name)
{
	//注销队列
	std::map<string,GentReplication*>::iterator it = rep_list_.find(name);
	if(it == rep_list_.end()) return true;
	if(!rep_list_[name]->SetLogout()) return false;
	pthread_attr_t attr;
	pthread_t pid;
	pthread_attr_init(&attr);
	GentReplication *r = rep_list_[name];
	int ret = pthread_create(&pid,&attr,GentRepMgr::ClearRep,r);
	if(ret != 0) { 
		LOG(GentLog::ERROR, "start clear %s replication failed.", name.c_str());	
		return false;
	}
	rep_list_.erase(it);
	return true;
}

void *GentRepMgr::ClearRep(void *arg) 
{
	LOG(GentLog::WARN, "start clearRep replication.");
	GentReplication *r = static_cast<GentReplication *>(arg);	
	delete r;
	return ((void *)0);
}

void GentRepMgr::Init() 
{
	map<string,repinfo *>::iterator it;
	for(it=rep_map_.begin(); it!=rep_map_.end();it++) {
		rep_list_[it->first] = new GentReplication(it->first, it->second);
	}
}

GentReplication *GentRepMgr::Get(const string &name, bool is_sync_all)
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
		rep_list_[name] = new GentReplication(name, info, is_sync_all);
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

GentReplication::GentReplication(const string &name, repinfo *rinfo, bool is_sync_all):status(0),
main_que_length(0)
{
	is_run = true;
	slave_start_time = time(NULL);	
	rep_name = name;
	rinfo_ = rinfo;
	current_node = NULL;
	if(!rinfo_->ser_time || rinfo_->ser_time > rinfo_->rep_time) {
		if(is_sync_all) {
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
		}
	}else{
		LOG(GentLog::WARN, "not need to sync all data for %s slave.", name.c_str());
	}
}

GentReplication::~GentReplication(){
	while(main_que_length>0) {
		Pop(false);
	}
}

void GentReplication::Push(int type, string &key)
{
	if(is_run == false) return;
	AutoLock lock(&que_push_lock);
	itemData *item = new itemData(key,type);
	main_que.push(item);
	//通知slave
	if(main_que_length == 0 && conn_ && conn_->fd>0) {
		GentThread::Intance()->SendMasterMsg(this);
	}
	
	main_que_length++;
	rinfo_->set("ser_time",time(NULL));
}

void GentReplication::Pop(bool is_rep)
{
	AutoLock lock(&que_push_lock);
	itemData *it;
	int num = 1;
	int max = SYNC_NUM*2;
	if(!is_update_que) {
		while(num <= max) {
			it = main_que.front_element();
			if(it == NULL || it->is_sync == false) break; 
			it= main_que.pop();
	 		if(is_rep == true) {
				rinfo_->set("rep_time",time(NULL));
			}
			delete it;
			it = NULL;
			main_que_length--;
			num++;
			
		}
	}else{
		while(num <= max) {
			it = que.front_element();
			if(it == NULL) break;
			if(it->is_sync == false) break;
			it= que.pop();
			if(it == NULL && !rinfo_->rep_time ) {
				if(is_rep == true) {
					rinfo_->set("rep_time", time(NULL));
				}
			}
			delete it;
			it = NULL;
			main_que_length--;
			num++;
		}
	}
	if(it && it->is_sync) delete it;
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

bool GentReplication::front_nums_element(std::vector<itemData *> &dat, int num)
{
	AutoLock lock(&que_push_lock);
	if(!is_update_que) {
		return main_que.front_nums_element(dat, num);
	}
	if(!que.front_nums_element(dat, num)){
		is_update_que = false;
		if(!rinfo_->rep_time ) {
			rinfo_->set("rep_time", time(NULL));	
		}
		return main_que.front_nums_element(dat, num);
	}
	return true;
}


bool GentReplication::SetLogout()
{
	if(conn_) return false;
	is_run = false;
	rinfo_->set("available",0);
	return true;
}

void GentReplication::GetInfo(string &str)
{

	string s = "";
	if(!conn_) {
		s = "close";
	}else{
		s = conn_->GetStatus();
	}
	char ret[500]={0};
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
	c->RegDestroy(this);
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
	outstr = "";
	//同步数据
	vector<itemData *> syncData;
	if(!front_nums_element(syncData, SYNC_NUM)) {
		outstr = "";
		return false;
	}
	status = 1;
	vector<itemData *>::iterator iter;
	int msetNum = 0;
	for(iter = syncData.begin(); iter != syncData.end(); iter++) {
		itemData *it = *iter;
		if(it->type != itemData::ADD && msetNum != 0) return MsetReply(outstr, msetNum);
		if(it->type != itemData::ADD) {
			Reply(itemData::DEL, it->name, outstr);
			it->setSync();
			return true;
		}
		string nr = "";
		LOG(GentLog::INFO, "sync the key of %s data.", it->name.c_str());
		uint64_t expire;
		if(!GentDb::Instance()->Get(it->name, nr, expire)) {
			if(msetNum == 0) {
				Reply(itemData::DEL, it->name,	outstr);
				it->setSync();
				return true;
			}
			return MsetReply(outstr, msetNum);		
		}
		uint64_t t = (unsigned long long)time(NULL);
		if(expire != 0 && expire < t) {
			if(msetNum == 0) {
				Reply(itemData::DEL, it->name,	outstr);
				it->setSync();
				return true;
			}
			return MsetReply(outstr, msetNum);
		}
		if(expire != 0) {
			if(msetNum > 0) return MsetReply(outstr, msetNum);
			expire = expire - t;
			LOG(GentLog::INFO, "reply %s.", it->name.c_str());
			Reply(itemData::ADD, it->name, outstr, nr, expire);	
			it->setSync();
			return true;
		}
		string itstr; 
		if(it->name != "") {
			ReplyItem(it->name, itstr, nr);
			outstr += itstr;
			msetNum++;
		}
		it->setSync();	
	}
	return MsetReply(outstr, msetNum);	
}

bool GentReplication::MsetReply(string &outstr, int num)
{
	char headstr[30]={0};
	snprintf(headstr,30,"*%d\r\n$4\r\nmset\r\n", num*2+1);
	outstr = headstr + outstr;
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

void GentReplication::ReplyItem(string &key,string &outstr, const string &nr)
{
	char retstr[100] = {0};	
	snprintf(retstr,100,"$%ld\r\n%s\r\n$%ld\r\n",
				(unsigned long)key.size(),key.c_str(),(unsigned long)nr.size());

	outstr=retstr+nr+"\r\n";
}



