#include "prefine.h"
#include "gent_util.h"
#include "gent_redis.h"
#include "gent_db.h"
#include "gent_list.h"
#include "gent_app_mgr.h"

GentRedis::GentRedis(GentConnect *c):GentCommand(c)
{
  keystr = "";
  rlbytes = 0;
}
GentRedis::~GentRedis(){}

int GentRedis::ParseCommand(const string &data)
{
	vector<string> tokenList;
	int num = Split(data, "\r\n",	tokenList);
	if(num < 3) return -1;
	int fieldNum = GetLength(tokenList[0]);
	int len = fieldNum*2 + 1; 
	//if(num != len) {
	//	cout << "string format Error" <<endl;
	//	return -1;
	//}	
	if(tokenList[2] == "SET" || tokenList[2] == "set") {
		conn->SetStatus(Status::CONN_NREAD);
		if(num < 5) return -1;	
		keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
		if(keystr.size()>=250) return -3;
		int pos = 0;
		int i = 5;
		while(i>=0) {
			pos = data.find_first_of("\r\n", pos);
			if(pos == string::npos) return -1;
			pos+=2;
			i--;
		}
		rlbytes = GetLength(tokenList[5]);
		content = data.substr(pos);
		commandtype = CommandType::COMM_SET;
		if(rlbytes<content.length()) {
			content = data.substr(pos, rlbytes);
			return 0;
		}
		return rlbytes-content.length();			
	}else if(tokenList[2] == "get" || tokenList[2] == "GET") {
		if(num != 5) return -1; 
		conn->SetStatus(Status::CONN_DATA);
		commandtype = CommandType::COMM_GET;	
		keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
		return 0;
	}else if(tokenList[2] == "mget" || tokenList[2] == "MGET") {
		if(num < 5 || num != len) return -1;
		keyvec.clear();	
		conn->SetStatus(Status::CONN_DATA);
		commandtype = CommandType::COMM_MGET;
		for(int i=1; i<fieldNum; i++) {
			int lenKey = i*2+1;
			keyvec.push_back(tokenList[lenKey+1].substr(0,GetLength(tokenList[lenKey])));
		}	
		return 0;		
	}else if(tokenList[2] == "del" || tokenList[2] == "DEL") {
		if(num != 5) return -1; 
		conn->SetStatus(Status::CONN_DATA);
		commandtype = CommandType::COMM_DEL;
		keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
		return 0;
	}else if(tokenList[2] == "quit" || tokenList[2] == "QUIT") {
		conn->SetStatus(Status::CONN_CLOSE);
		return 0;
	}else if(tokenList[2] == "stats" || tokenList[2] == "STATS") {
		keystr = "";
		if(num == 5) {
			keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
		}
		commandtype = CommandType::COMM_STATS;
		conn->SetStatus(Status::CONN_DATA);
		return 0;
	}else if(tokenList[2] == "keys" || tokenList[2] == "KEYS") {
		cout << "num: " << num <<endl;
		if(num != 5) return -2;
		keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
		commandtype = CommandType::COMM_KEYS;
		conn->SetStatus(Status::CONN_DATA);
		return 0;
	}else if(tokenList[2] == "exists" || tokenList[2] == "EXISTS") {
		if(num != 5) return -1; 
		conn->SetStatus(Status::CONN_DATA);
		commandtype = CommandType::COMM_EXISTS;	
		keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
		return 0;
	}
	return -1;

}

int GentRedis::Process(const char *rbuf, size_t size, string &outstr) 
{
	string data = string(rbuf,size);
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
            last_pos++;                                                                
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

void GentRedis::ProcessDel(string &outstr)
{
	if(!GentList::Instance()->Load(keystr)) {
		outstr = ":0\r\n";
		return;
	}
	string nr = "";
	if(!GentDb::Instance()->Get(keystr, nr)){
		outstr = ":0\r\n";
		return;                  
	}
	GentList::Instance()->Clear(keystr);			
	if(!GentDb::Instance()->Del(keystr)) {
		outstr = ":0\r\n";
	}else{
		outstr = ":1\r\n";
	}
}

void GentRedis::ProcessSet(string &outstr, const char *recont, uint64_t len)
{
	LOG(GentLog::WARN, "commandtype::comm_set");
	content += string(recont,len);
	string nr;                   
	nr.assign(content.c_str(), rlbytes);
	if(!GentDb::Instance()->Put(keystr, nr)) {
		outstr = Info("NOT_STORED",REDIS_ERROR);
	}else{
		//GentList::Instance()->Save(keystr);
		LOG(GentLog::WARN, "commandtype::comm_set stored");
		outstr=REDIS_INFO+"\r\n";
	}

}

void GentRedis::ProcessGet(string &outstr)
{
    string nr="";
    if(!GentDb::Instance()->Get(keystr, nr))
    {
        outstr = "$-1\r\n";
    }
    char retbuf[50]={0};
    snprintf(retbuf,50, "$%ld\r\n",nr.size());
    outstr = retbuf;
	outstr += nr+"\r\n";
}

void GentRedis::ProcessExists(string &outstr)
{
    string nr="";
    if(!GentDb::Instance()->Get(keystr, nr))
    {
        outstr = ":0\r\n";
    }else{
		outstr = ":1\r\n";
	}
}


void GentRedis::ProcessMultiGet(string &outstr)
{
    vector<string>::iterator iter;
	outstr = "";
	int num = 0;
    for(iter=keyvec.begin(); iter!=keyvec.end(); iter++)
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

void GentRedis::ProcessStats(string &outstr)
{
    char retbuf[200] = {0};
	uint64_t num = GentDb::Instance()->Count(keystr);
    uint64_t totals = GentDb::Instance()->TotalSize(); 
	snprintf(retbuf,200,"total_connect: %lu\r\ncurrent_connect: %lu\r\nitem_nums: %lu\r\ntotal_size: %lu\r\n",
             GentAppMgr::Instance()->GetTotalConnCount(),GentAppMgr::Instance()->GetConnCount(), num, totals);
    outstr = retbuf;
}

void GentRedis::ProcessKeys(string &outstr)
{
	vector<string> outvec;
	vector<string>::iterator it;
	GentDb::Instance()->Keys(outvec, keystr);
	char ret[20]={0};
	snprintf(ret,20,"*%ld\r\n",outvec.size());
	outstr = ret;	
	for(it=outvec.begin();it!=outvec.end();it++) {
		char s[260]={0};
		snprintf(s,260,"$%ld\r\n%s\r\n",(*it).size(),(*it).c_str());
		outstr+=string(s);
	}
}

void GentRedis::Complete(string &outstr, const char *recont, uint64_t len)
{
	switch(commandtype)
	{
		case CommandType::COMM_GET:
			//NOT_FOUND
            //if(keystr!="" && !GentList::Instance()->Load(keystr)) {
			//	outstr += "END\r\n";
			//	break;
			//}
            ProcessGet(outstr);
			break;	
		case CommandType::COMM_SET:
			ProcessSet(outstr, recont, len);	
			break;
		case CommandType::COMM_QUIT:
			break;
		case CommandType::COMM_DEL:
			ProcessDel(outstr); 
			break;
        case CommandType::COMM_STATS:
            ProcessStats(outstr);
            break;
		case CommandType::COMM_KEYS:
			ProcessKeys(outstr);
			break;
		case CommandType::COMM_MGET:
			ProcessMultiGet(outstr);
			break;
		case CommandType::COMM_EXISTS:
			ProcessExists(outstr);
			break;
		default:
			outstr = Info("command",REDIS_ERROR);
			return;
	}		

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
   GentList::Instance()->Init();
   return true;
}

