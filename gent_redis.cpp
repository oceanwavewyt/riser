#include "prefine.h"
#include "gent_util.h"
#include "gent_redis.h"
#include "gent_db.h"
#include "gent_list.h"
#include "gent_app_mgr.h"
#include "gent_frame.h"
#include "gent_repl.h"

std::map<string, GentSubCommand*> GentRedis::commands;

bool GentSubCommand::IsAuth(GentRedis *r)
{
	if(r->GetAuth()) return true;
	GentConfig &config = GentFrame::Instance()->config;
	if(config["auth"] == "") {
		r->SetAuth(1);
		return true;	
	}
	return false;
}

GentRedis::GentRedis(GentConnect *c):GentCommand(c)
{
  keystr = "";
  auth = 0;
  rlbytes = 0;
  expire = 0;
}
GentRedis::~GentRedis(){
}

void GentRedis::SetCommands()
{
	GentProcessGet *g=new GentProcessGet();	
	commands["get"] = g;
	commands["GET"] = g;
	GentProcessSet *s=new GentProcessSet();	
	commands["set"] = s;
	commands["SET"] = s;
	GentProcessSetex *ex=new GentProcessSetex();	
	commands["setex"] = ex;
	commands["SETEX"] = ex;
	GentProcessAuth *ah=new GentProcessAuth();	
	commands["auth"] = ah;
	commands["AUTH"] = ah;
	GentProcessTtl *ttl=new GentProcessTtl();	
	commands["ttl"] = ttl;
	commands["TTL"] = ttl;
	GentProcessMget *m=new GentProcessMget();	
	commands["mget"] = m;
	commands["MGET"] = m;
	GentProcessDel *del=new GentProcessDel();	
	commands["del"] = del;
	commands["DEL"] = del;
	GentProcessQuit *quit=new GentProcessQuit();	
	commands["quit"] = quit;
	commands["QUIT"] = quit;
	GentProcessKeys *keys=new GentProcessKeys();	
	commands["keys"] = keys;
	commands["KEYS"] = keys;
	GentProcessExists *e=new GentProcessExists();	
	commands["exists"] = e;
	commands["EXISTS"] = e;
	GentProcessPing *p=new GentProcessPing();	
	commands["ping"] = p;
	commands["PING"] = p;
	GentProcessInfo *info=new GentProcessInfo();	
	commands["info"] = info;
	commands["INFO"] = info;
	GentProcessRep *rep=new GentProcessRep();	
	commands["rep"] = rep;
	commands["REP"] = rep;
	GentProcessReply *reply=new GentProcessReply();	
	commands["reply"] = reply;
	commands["REPLY"] = reply;
	GentProcessSlave *sl=new GentProcessSlave();	
	commands["slave"] = sl;
	commands["SLAVE"] = sl;
}

int GentProcessSlave::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(!IsAuth(redis)) return AUTH_REQ_FAIL; 
	if(num != 7) return -1;
	string act = tokenList[6].substr(0,GetLength(tokenList[5]));
	if(act != "clear") return -1;
	redis->conn->SetStatus(Status::CONN_DATA);
	redis->keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
	return 0;
}

void GentProcessSlave::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	if(!GentRepMgr::Instance("master")->Logout(redis->keystr)) {
		redis->Info(redis->keystr+" is run,failed to stop", outstr , REDIS_ERROR);		
	}else{
		outstr = "+OK\r\n";
	}
}

int GentProcessGet::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(!IsAuth(redis)) return AUTH_REQ_FAIL;
	if(num != 5) return -1; 
	redis->conn->SetStatus(Status::CONN_DATA);
	redis->keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
	return 0;
}

void GentProcessGet::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	string nr="";
    if(!GentDb::Instance()->Get(redis->keystr, nr))
    {
        outstr = "$-1\r\n";
    }
    char retbuf[50]={0};
    snprintf(retbuf,50, "$%u\r\n",(unsigned int)nr.size());
    outstr = retbuf;
	outstr += nr+"\r\n";
}

int GentProcessSet::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(!IsAuth(redis)) return AUTH_REQ_FAIL;
	redis->conn->SetStatus(Status::CONN_NREAD);
	if(num < 5) return -1;	
	string keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
	redis->keystr = keystr;
	if(keystr.size()>=250) return -3;
	size_t pos = data.find_first_of("*3",0);
	if(pos == string::npos) return -1;
	int i = 5;
	while(i>=0) {
		pos = data.find_first_of("\r\n", pos);
		if(pos == string::npos) return -1;
		pos+=2;
		i--;
	}
	uint64_t rlbytes = GetLength(tokenList[5]);
	redis->content = data.substr(pos);
	redis->rlbytes = rlbytes;
	if(rlbytes < redis->content.length()) {
		redis->content = data.substr(pos, rlbytes);
		return 0;
	}
	return rlbytes-redis->content.length();
}

void GentProcessSet::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	LOG(GentLog::WARN, "commandtype::comm_set");
	redis->content += string(recont,len);
	if(!GentDb::Instance()->Put(redis->keystr, redis->content.c_str(), redis->rlbytes, 0)) {
		if(!redis->Slave()) {
			redis->Info("NOT_STORED",outstr, REDIS_ERROR);
		}else{
			GentRepMgr::Instance("slave")->SlaveReply(outstr, 0);	
		}
	}else{
		//GentList::Instance()->Save(keystr);
		GentRepMgr::Instance("master")->Push(itemData::ADD, redis->keystr);
		LOG(GentLog::INFO, "it is sucess for %s stored",redis->keystr.c_str());
		if(!redis->Slave()) {
			outstr=REDIS_INFO+"\r\n";
		}else{
			GentRepMgr::Instance("slave")->SlaveReply(outstr, 1);	
		}
	}
}

int GentProcessSetex::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(!IsAuth(redis)) return AUTH_REQ_FAIL;
	redis->conn->SetStatus(Status::CONN_NREAD);
	if(num < 7) return -1;	
	string keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
	redis->keystr = keystr;
	if(keystr.size()>=250) return -3;
	size_t pos = data.find_first_of("*4",0);
	if(pos == string::npos) return -1;
	int i = 7;
	while(i>=0) {
		pos = data.find_first_of("\r\n", pos);
		if(pos == string::npos) return -1;
		pos+=2;
		i--;
	}
	//expire
	redis->expire = atoi(tokenList[6].c_str());
	redis->expire += time(NULL);
	uint64_t rlbytes = GetLength(tokenList[7]);
	redis->content = data.substr(pos);
	redis->rlbytes = rlbytes;
	if(rlbytes < redis->content.length()) {
		redis->content = data.substr(pos, rlbytes);
		return 0;
	}
	return rlbytes-redis->content.length();
}

void GentProcessSetex::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	redis->content += string(recont,len);
	if(!GentDb::Instance()->Put(redis->keystr, redis->content.c_str(), redis->rlbytes, redis->expire)) {
		if(!redis->Slave()) {
			redis->Info("NOT_STORED", outstr, REDIS_ERROR);
		}else{
			GentRepMgr::Instance("slave")->SlaveReply(outstr, 0);	
		}
	}else{
		//GentList::Instance()->Save(keystr);
		GentRepMgr::Instance("master")->Push(itemData::ADD, redis->keystr);
		LOG(GentLog::INFO, "it is sucess for %s stored",redis->keystr.c_str());
		if(!redis->Slave()) {
			outstr=REDIS_INFO+"\r\n";
		}else{
			GentRepMgr::Instance("slave")->SlaveReply(outstr, 1);	
		}
	}
}


int GentProcessMget::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(!IsAuth(redis)) return AUTH_REQ_FAIL;
	int fieldNum = GetLength(tokenList[0]);
	int len = fieldNum*2 + 1;
	if(num < 5 || num != len) return -1;
	redis->keyvec.clear();
	redis->conn->SetStatus(Status::CONN_DATA);
	for(int i=1; i<fieldNum; i++) {
		int lenKey = i*2+1;
		redis->keyvec.push_back(tokenList[lenKey+1].substr(0,GetLength(tokenList[lenKey])));
	}	
	return 0;
}

void GentProcessMget::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	vector<string>::iterator iter;
	outstr = "";
	int num = 0;
    for(iter=redis->keyvec.begin(); iter!=redis->keyvec.end(); iter++)
    {
        string nr="";
        if(!GentDb::Instance()->Get(*iter, nr))
        {
            continue;
        }
		num++;
        char retbuf[20]={0};
        snprintf(retbuf,20, "$%u\r\n", (unsigned int)nr.size());
        outstr += retbuf;
        outstr += nr+"\r\n";
    }
	char ret[20]={0};
	snprintf(ret,20,"*%d\r\n",num);
	outstr = ret + outstr;
}

int GentProcessTtl::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(!IsAuth(redis)) return AUTH_REQ_FAIL;
	if(num != 5) return -1; 
	redis->conn->SetStatus(Status::CONN_DATA);
	redis->keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
	return 0;
}

void GentProcessTtl::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	string keystr = redis->keystr;
	uint64_t expire;
	bool isTtl= GentDb::Instance()->Ttl(keystr, expire); 
	outstr = ":-2\r\n";
	if(isTtl && expire > 0) {
		if(expire > (unsigned long long)time(NULL)) {
			char buf[50]={0};
			snprintf(buf, 50, ":%llu\r\n", (unsigned long long)(expire-time(NULL)));
			outstr = buf;
		}else{
			//expired
			outstr = ":-2\r\n";	
		}
	}else if(isTtl && expire == 0) {
		outstr = ":-1\r\n";
	}
}

int GentProcessDel::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(!IsAuth(redis)) return AUTH_REQ_FAIL;
	if(num != 5) return -1; 
	redis->conn->SetStatus(Status::CONN_DATA);
	redis->keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
	return 0;
}

void GentProcessDel::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	string keystr = redis->keystr;
	//if(!GentList::Instance()->Load(keystr)) {
	//	outstr = ":0\r\n";
	//	return;
	//}
	string nr = "";
	if(!GentDb::Instance()->Get(keystr, nr)){
		outstr = ":0\r\n";
		if(redis->Slave()) {
			GentRepMgr::Instance("slave")->SlaveReply(outstr, 1);
		}
		return;                  
	}
	//GentList::Instance()->Clear(keystr);			
	if(!GentDb::Instance()->Del(keystr)) {
		outstr = ":0\r\n";
		if(redis->Slave()) {
			GentRepMgr::Instance("slave")->SlaveReply(outstr, 0);
		}
	}else{
		GentRepMgr::Instance("master")->Push(itemData::DEL, keystr);
		outstr = ":1\r\n";
		if(redis->Slave()) {
			GentRepMgr::Instance("slave")->SlaveReply(outstr, 1);
		}
	}
}

int GentProcessInfo::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(!IsAuth(redis)) return AUTH_REQ_FAIL;
	redis->keystr = "";
	if(num >= 5) {
		redis->keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
	}
	redis->conn->SetStatus(Status::CONN_DATA);
	return 0;
}

void GentProcessInfo::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	char retbuf[500] = {0};
	if(redis->keystr == "rep") {
		string master_info;
		GentRepMgr::Instance("master")->GetSlaveInfo(master_info);
		string slave_info;
		GentRepMgr::Instance("slave")->GetInfo(slave_info);
		snprintf(retbuf,500,
			"# Master\r\n"
			"%s\r\n"
			"# Slave\r\n"
			"%s\r\n"
			,
			 master_info.c_str(),	
			 slave_info.c_str()
			 );
	}else {
	    uint64_t totals = GentDb::Instance()->TotalSize(); 
		char hmen[64]={0};
		GentConfig &config = GentFrame::Instance()->config;
		string role = "master";
		uint32_t slaveNum = GentRepMgr::Instance("master")->GetReplicationNum();
		if(config["slaveof_ip"] != "" && config["slaveof_port"] != "") {
			role += (role == "")?"slave":",slave";
		}
			
		GentUtil::BytesToHuman(hmen, totals);
		snprintf(retbuf,500,"# Server\r\n"
				"process_id: %ld\r\n"
				"port: %d\r\n"
				"config_file: %s\r\n"
				"\r\n# Clients\r\n"
				"total_connect: %u\r\n"
				"current_connect: %u\r\n"
				"\r\n# Disk\r\n"
				"disk_use: %llu\r\n"
				"disk_use_human: %s\r\n"
				"\r\n# Keyspace\r\n"
				"key_num: %llu\r\n"
				"\r\n# Replication\r\n"
				"role:%s\r\n"
				"connected_slaves:%u\r\n"
				"master_repl_length:%llu\r\n\r\n"
				,
	             (long) getpid(),
				 GentFrame::Instance()->s->port,
				 GentFrame::Instance()->s->configfile,
				 (unsigned int)GentAppMgr::Instance()->GetTotalConnCount(),
				 (unsigned int)GentAppMgr::Instance()->GetConnCount(),
				 (unsigned long long)totals,
				 hmen,
				 (unsigned long long)GentDb::Instance()->Count(""),
				 role.c_str(),
				 slaveNum,
				 (unsigned long long)GentRepMgr::Instance("master")->QueLength()
				 );
	}
	outstr = retbuf;
    char c[50]={0};
	snprintf(c,50,"$%u\r\n",(unsigned int)outstr.size()-2);
	outstr = c+outstr;
	return;
}

int GentProcessQuit::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	redis->conn->SetStatus(Status::CONN_CLOSE);	
	return 0;
}

void GentProcessQuit::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	return;
}

int GentProcessKeys::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(!IsAuth(redis)) return AUTH_REQ_FAIL;
	if(num != 5) return -2;
	redis->keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
	redis->conn->SetStatus(Status::CONN_DATA);
	return 0;
}

void GentProcessKeys::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	vector<string> outvec;
	vector<string>::iterator it;
	GentDb::Instance()->Keys(outvec, redis->keystr);
	char ret[20]={0};
	snprintf(ret,20,"*%u\r\n",(unsigned int)outvec.size());
	outstr = ret;	
	for(it=outvec.begin();it!=outvec.end();it++) {
		char s[260]={0};
		snprintf(s,260,"$%u\r\n%s\r\n",(unsigned int)(*it).size(),(*it).c_str());
		outstr+=string(s);
	}
}

int GentProcessExists::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(!IsAuth(redis)) return AUTH_REQ_FAIL;
	if(num != 5) return -1; 
	redis->conn->SetStatus(Status::CONN_DATA);
	redis->keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
	return 0;
}

void GentProcessExists::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	string nr="";
    if(!GentDb::Instance()->Get(redis->keystr, nr))
    {
        outstr = ":0\r\n";
    }else{
		outstr = ":1\r\n";
	}
}

int GentProcessPing::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(!IsAuth(redis)) return AUTH_REQ_FAIL;
	redis->conn->SetStatus(Status::CONN_DATA);
	return 0;
}

void GentProcessPing::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	outstr="+PONG\r\n";	
}

int GentProcessRep::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(num < 7) return -1;
	//rep name auth authstr
	//rep name ok
	//验证auth是否合法
	redis->repmsg = tokenList[6];
	//LOG(GentLog::INFO, "fd:%d,GentProcessRep::Parser: %s",redis->conn->fd,msg.c_str());
	if(redis->master_auth == 0) {
		GentConfig &config = GentFrame::Instance()->config;
		if(config["master_auth"] == "") {
			redis->master_auth = 1;
		}else{			
			if(num != 9 || tokenList[6] != "auth" || 
					tokenList[8] != config["master_auth"]) {
				//msg = "autherror";
				redis->repmsg = "autherror";
			}else{
				redis->master_auth = 1;
			}
		}
	}
	
	//LOG(GentLog::INFO, "fd:%d,tokenList[4] pre: %s, length:%d",redis->conn->fd,tokenList[4].c_str(),GetLength(tokenList[3]));	
	redis->keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
	redis->conn->SetStatus(Status::CONN_DATA);
	return 0;
}

void GentProcessRep::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	GentReplication *rep = GentRepMgr::Instance("master")->Get(redis->keystr);
	if(rep == NULL) {
		outstr = REDIS_ERROR+" slave full,manual clear\r\n";
		return;
	}
	if(!rep->Start(redis->repmsg, redis->conn, outstr)) {
		redis->conn->SetStatus(Status::CONN_WAIT);
	}
}

int GentProcessReply::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
		if(!IsAuth(redis)) return AUTH_REQ_FAIL;
		if(num == 5 && tokenList[4] == "close") {
			redis->conn->SetStatus(Status::CONN_CLOSE);	
		}else if(num == 5 && tokenList[4] == "complete") {
			redis->conn->SetStatus(Status::CONN_WAIT);
			GentRepMgr::Instance("slave")->SlaveSetStatus(GentRepMgr::CONTINUE);
		}else if(num == 5  && tokenList[4] == "authok") {
			LOG(GentLog::INFO, "auth success");
			redis->conn->SetStatus(Status::CONN_WAIT);
			GentRepMgr::Instance("slave")->SlaveSetStatus(GentRepMgr::CONTINUE);
		}else if(num == 5  && tokenList[4] == "autherror") {
			LOG(GentLog::ERROR, "slave auth failed");	
			redis->conn->SetStatus(Status::CONN_WAIT);	
		}
		return 0;
	return 0;
}

void GentProcessReply::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{

}

int GentProcessAuth::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
	if(num != 5) return -1;
	string str = tokenList[4].substr(0,GetLength(tokenList[3]));	
	GentConfig &config = GentFrame::Instance()->config;
	if(config["auth"] == "") {
		redis->SetAuth(1);
		return 0;	
	}
	if(str != config["auth"])  return AUTH_FAIL;
	redis->conn->SetStatus(Status::CONN_DATA);	
	return 0;
}

void GentProcessAuth::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	redis->SetAuth(1);
	outstr = "+OK\r\n"; 		
}


int GentRedis::ParseCommand(const string &data)
{
    if(data.size()==0) return 0;
	vector<string> tokenList;
	int num = Split(data, "\r\n",	tokenList);
	if(num==0) return 0;
	if(num < 3) return -1;
	std::map<string, GentSubCommand*>::iterator it=commands.find(tokenList[2]);
/*
	if(subc) {
		delete subc;
		subc = NULL;
	}
*/
	if(it == commands.end()){
		return -1;
	}
	//subc = (*it).second->Clone();
	subc = (*it).second;
	return subc->Parser(num, tokenList, data, this);
}

int GentRedis::Process(const char *rbuf, uint64_t size, string &outstr)
{
	string data(rbuf,size);
	int status = ParseCommand(data);
	if(status == -1) {
		outstr = REDIS_ERROR + " unknown command\r\n";
		//Info("unknown command",outstr, REDIS_ERROR);
		return 0;
	}else if(status == -2) {
		outstr = REDIS_ERROR + " wrong number of arguments for 'keys' command\r\n";
		//Info("wrong number of arguments for 'keys' command",outstr, REDIS_ERROR);
		return 0;
	}else if(status == -3) {
		outstr = REDIS_ERROR + " key is very very long\r\n";
		//Info("key is very very long",outstr, REDIS_ERROR);
		return 0;
	}else if(status == AUTH_REQ_FAIL) {
		outstr = "- " + REDIS_AUTH_REQ + "\r\n";
		//Info(REDIS_AUTH_REQ, outstr, "-");
		return 0;
	}else if(status == AUTH_FAIL) {
		outstr = REDIS_ERROR + " " + REDIS_AUTH_FAIL + "\r\n";
		//Info(REDIS_AUTH_FAIL,outstr, REDIS_ERROR);
		return 0;
	}
	return status;
}

uint64_t GentRedis::GetLength(string &str) 
{
	return atoi(str.substr(1).c_str());
}
int GentRedis::Split(const string &str, const string &delimit, vector<string> &v) {
    uint64_t pos,last_pos=0;                                                           
    int num = 0;                                                                   
    while((pos = str.find_first_of(delimit,last_pos)) != string::npos){                
        if(pos == last_pos){                                                           
            last_pos+=2;                                                                
        }else{
            v.push_back(str.substr(last_pos, pos-last_pos));                           
            num++;                                                                     
            last_pos = pos+2;                                                          
        }                                                                              
    }
	return num;	
}                                                                                  

void GentRedis::Info(const string &msg, string &outstr, const string &type)
{
	if(msg == "") {
		outstr = type+"\r\n";
	}else{
		outstr = type+" "+msg+"\r\n";
	}	
}



void GentRedis::ProcessStats(string &outstr)
{
    char retbuf[200] = {0};
	uint64_t num = GentDb::Instance()->Count(keystr);
    uint64_t totals = GentDb::Instance()->TotalSize(); 
	snprintf(retbuf,200,"total_connect: %u\r\ncurrent_connect: %u\r\nitem_nums: %llu\r\ntotal_size: %llu\r\n",
             (unsigned int)GentAppMgr::Instance()->GetTotalConnCount(),
			 (unsigned int)GentAppMgr::Instance()->GetConnCount(), 
			 (unsigned long long)num, 
			 (unsigned long long)totals);
    outstr = retbuf;
}

void GentRedis::Complete(string &outstr, const char *recont, uint64_t len)
{
	if(subc == NULL) {
		Info("command",outstr, REDIS_ERROR);
		return;
	}
	subc->Complete(outstr, recont, len, this);
	return;
}
GentCommand *GentRedis::Clone(GentConnect *connect)
{
	return new GentRedis(connect);
}
bool GentRedis::Init(string &msg)
{
   if(!GentDb::Instance()->Init(msg))
   {
       LOG(GentLog::ERROR, "db init fail,%s",msg.c_str());
       return false;
   }
   GentRedis::SetCommands();
   //GentList::Instance()->Init();
   return true;
}

bool GentRedis::Slave()
{
	return conn->is_slave;
}

