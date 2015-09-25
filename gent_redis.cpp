#include "prefine.h"
#include "gent_util.h"
#include "gent_redis.h"
#include "gent_db.h"
#include "gent_list.h"
#include "gent_app_mgr.h"
#include "gent_frame.h"
#include "gent_repl.h"

std::map<string, GentSubCommand*> GentRedis::commands;

GentRedis::GentRedis(GentConnect *c):GentCommand(c)
{
  keystr = "";
  auth = "";
  rlbytes = 0;
  c = NULL;
}
GentRedis::~GentRedis(){}

void GentRedis::SetCommands()
{
	GentProcessGet *g=new GentProcessGet();	
	commands["get"] = g;
	commands["GET"] = g;
	GentProcessSet *s=new GentProcessSet();	
	commands["set"] = s;
	commands["SET"] = s;
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
	redis->conn->SetStatus(Status::CONN_DATA);
	return 0;
}

void GentProcessSlave::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{
	outstr = "+OK\r\n";
	cout << outstr<<endl;
}

int GentProcessGet::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
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
    snprintf(retbuf,50, "$%ld\r\n",nr.size());
    outstr = retbuf;
	outstr += nr+"\r\n";
}

int GentProcessSet::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
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
	if(!GentDb::Instance()->Put(redis->keystr, redis->content.c_str(), redis->rlbytes)) {
		if(!redis->Slave()) {
			outstr = redis->Info("NOT_STORED",REDIS_ERROR);
		}else{
			GentRepMgr::Instance("slave")->SlaveReply(outstr, 0);	
		}
	}else{
		//GentList::Instance()->Save(keystr);
		GentRepMgr::Instance("master")->Push(itemData::ADD, redis->keystr);
		LOG(GentLog::WARN, "it is sucess for %s stored",redis->keystr.c_str());
		if(!redis->Slave()) {
			outstr=REDIS_INFO+"\r\n";
		}else{
			GentRepMgr::Instance("slave")->SlaveReply(outstr, 1);	
		}
	}
}

int GentProcessMget::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
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
        snprintf(retbuf,20, "$%ld\r\n", nr.size());
        outstr += retbuf;
        outstr += nr+"\r\n";
    }
	char ret[20]={0};
	snprintf(ret,20,"*%d\r\n",num);
	outstr = ret + outstr;
}

int GentProcessDel::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
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
			GentRepMgr::Instance("slave")->SlaveReply(outstr, 0);
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
				"total_connect: %lu\r\n"
				"current_connect: %lu\r\n"
				"\r\n# Disk\r\n"
				"disk_use: %lu\r\n"
				"disk_use_human: %s\r\n"
				"\r\n# Keyspace\r\n"
				"key_num: %lu\r\n"
				"\r\n# Replication\r\n"
				"role:%s\r\n"
				"connected_slaves:%u\r\n"
				"master_repl_length:%lu\r\n\r\n"
				,
	             (long) getpid(),
				 GentFrame::Instance()->s->port,
				 GentFrame::Instance()->s->configfile,
				 GentAppMgr::Instance()->GetTotalConnCount(),
				 GentAppMgr::Instance()->GetConnCount(),
				 totals,
				 hmen,
				 GentDb::Instance()->Count(""),
				 role.c_str(),
				 slaveNum,
				 GentRepMgr::Instance("master")->QueLength()
				 );
	}
	outstr = retbuf;
    char c[50]={0};
	snprintf(c,50,"$%lu\r\n",outstr.size()-2);
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
	snprintf(ret,20,"*%ld\r\n",outvec.size());
	outstr = ret;	
	for(it=outvec.begin();it!=outvec.end();it++) {
		char s[260]={0};
		snprintf(s,260,"$%ld\r\n%s\r\n",(*it).size(),(*it).c_str());
		outstr+=string(s);
	}
}

int GentProcessExists::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
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
	msg = tokenList[6];
	if(redis->auth=="") {
		GentConfig &config = GentFrame::Instance()->config;
		if(config["master_auth"] == "") {
			redis->auth = "yes";
		}else{			
			if(num != 9 || tokenList[6] != "auth" || 
					tokenList[8] != config["master_auth"]) {
				msg = "autherror";
			}else{
				redis->auth = tokenList[8];
			}
		}
	}
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
	if(!rep->Start(msg, redis->conn, outstr)) {
		redis->conn->SetStatus(Status::CONN_WAIT);
	}
}

int GentProcessReply::Parser(int num,vector<string> &tokenList,const string &data,GentRedis *redis)
{
		if(num == 5 && tokenList[4] == "close") {
			redis->conn->SetStatus(Status::CONN_CLOSE);	
		}else if(num == 5 && tokenList[4] == "complete") {
			redis->conn->SetStatus(Status::CONN_WAIT);
			GentRepMgr::Instance("slave")->SlaveSetStatus(GentRepMgr::CONTINUE);
		}else if(num == 5  && tokenList[4] == "authok") {
			cout << "client authok.............." <<endl;
			redis->conn->SetStatus(Status::CONN_WAIT);
			GentRepMgr::Instance("slave")->SlaveSetStatus(GentRepMgr::CONTINUE);
		}else if(num == 5  && tokenList[4] == "autherror") {
			cout << "client auth failed" <<endl;
			LOG(GentLog::ERROR, "slave auth failed");	
			redis->conn->SetStatus(Status::CONN_WAIT);	
		}
		return 0;
	return 0;
}

void GentProcessReply::Complete(string &outstr,const char *recont, uint64_t len, GentRedis *redis)
{

}

int GentRedis::ParseCommand(const string &data)
{
    if(data.size()==0) return 0;
	vector<string> tokenList;
	int num = Split(data, "\r\n",	tokenList);
	if(num==0) return 0;
	if(num < 3) return -1;
	std::map<string, GentSubCommand*>::iterator it=commands.find(tokenList[2]);
	c = NULL;	
	if(it == commands.end()){
		return -1;
	}
	c = (*it).second;
	return c->Parser(num, tokenList, data, this);
}

int GentRedis::Process(const char *rbuf, uint64_t size, string &outstr)
{
	string data(rbuf,size);
	int status = ParseCommand(data);
	if(status == -1) {
		outstr = Info("unknown command",REDIS_ERROR);
		return 0;
	}else if(status == -2) {
		outstr = Info("wrong number of arguments for 'keys' command",REDIS_ERROR);
	}else if(status == -3) {
		outstr = Info("key is very very long",REDIS_ERROR);
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

string GentRedis::Info(const string &msg, const string &type)
{
	if(msg == "") {
		return type+"\r\n";
	}
	return type+" "+msg+"\r\n";	
}



void GentRedis::ProcessStats(string &outstr)
{
    char retbuf[200] = {0};
	uint64_t num = GentDb::Instance()->Count(keystr);
    uint64_t totals = GentDb::Instance()->TotalSize(); 
	snprintf(retbuf,200,"total_connect: %lu\r\ncurrent_connect: %lu\r\nitem_nums: %lu\r\ntotal_size: %lu\r\n",
             GentAppMgr::Instance()->GetTotalConnCount(),GentAppMgr::Instance()->GetConnCount(), num, totals);
    outstr = retbuf;
}

void GentRedis::Complete(string &outstr, const char *recont, uint64_t len)
{
	if(c == NULL) {
		outstr = Info("command",REDIS_ERROR);
		return;
	}
	c->Complete(outstr, recont, len, this);
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

