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
	if(num == 0) return -1;
	int fieldNum = GetLength(tokenList[0]);
	//int len = fieldNum*2 + 1; 
	//if(num != len) {
	//	cout << "string format Error" <<endl;
	//	return -1;
	//}	
	if(tokenList[2] == "SET" || tokenList[2] == "set") {
		conn->SetStatus(Status::CONN_NREAD);	
		keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
		int pos = 0;
		int i = 5;
		while(i>=0) {
			pos = data.find_first_of("\r\n", pos);
			if(pos == string::npos) return -1;
			pos+=2;
			i--;
		}
		rlbytes = GetLength(tokenList[5]);
		cout <<keystr << "   ppppppppppp:"<< pos<< "\trlbytes: "<<rlbytes <<endl;
		content = data.substr(pos);
		commandtype = CommandType::COMM_SET;
		if(rlbytes<content.length()) {
			content = data.substr(pos, rlbytes);
			return 0;
		}
		return rlbytes-content.length();			
	}else if(tokenList[2] == "GET" || tokenList[2] == "get") {
		conn->SetStatus(Status::CONN_DATA);
		commandtype = CommandType::COMM_GET;	
		keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
		return 0;
	}else if(tokenList[2] == "DEL" || tokenList[2] == "del") {
		conn->SetStatus(Status::CONN_DATA);
		commandtype = CommandType::COMM_DEL;
		keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
		return 0;
	}else if(tokenList[2] == "QUIT" || tokenList[2] == "quit") {
		conn->SetStatus(Status::CONN_CLOSE);
		return 0;
	}else if(tokenList[2] == "STATS" || tokenList[2] == "stats") {
		keystr = "";
		if(num == 5) {
			keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
		}
		commandtype = CommandType::COMM_STATS;
		conn->SetStatus(Status::CONN_DATA);
		return 0;
	}else if(tokenList[2] == "KEYS" || tokenList[2] == "keys") {
		if(num != 5) return -2;
		keystr = tokenList[4].substr(0,GetLength(tokenList[3]));
		commandtype = CommandType::COMM_KEYS;
		conn->SetStatus(Status::CONN_DATA);
		return 0;
	}
	return -1;

}

int GentRedis::Process(const char *rbuf, size_t size, string &outstr) 
{
	string data = string(rbuf,size);
	int status = ParseCommand(data);
	cout << "status:"<<status<<endl;	
	if(status == -1) {
		outstr = Info("unknown command",REDIS_ERROR);
		return 0;
	}else if(status == -2) {
		outstr = Info("wrong number of arguments for 'keys' command",REDIS_ERROR);
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

const string &GentRedis::Info(const string &msg, const string &type)
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
    if(keystr == "") {
        ProcessMultiGet(outstr);
        return;
    }
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

void GentRedis::ProcessMultiGet(string &outstr)
{
	/*
    vector<string>::iterator iter;
    for(iter=keys.begin(); iter!=keys.end(); iter++)
    {
        string nr="";
        if(!GentDb::Instance()->Get(*iter, nr))
        {
            continue;
        }
        char retbuf[200]={0};
        snprintf(retbuf,200, "VALUE %s 0 %ld\r\n",(*iter).c_str(), nr.size());
        outstr += retbuf;
        outstr += nr+"\r\n";
    }
	*/
    outstr += "END\r\n";
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
		char len[10]={0};
		printf("%ld",(*it).length());
		string s(len);
		outstr+="$"+s+"\r\n"+(*it)+"\r\n";
	}
}

void GentRedis::Complete(string &outstr, const char *recont, uint64_t len)
{
	cout << "GentRedis::Complete "<<len<<endl;
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
		default:
			outstr = "ERROR\r\n";
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



/*
int main() {
	string str="*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n";
	Redis *r=new Redis();
	string outstr="";	
	int c = r->Process(str.c_str(),str.length(), outstr);
	cout << "c:"<<c<<endl;
	delete r;
	return 1;
}
*/
