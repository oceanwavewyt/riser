
#include "prefine.h"
#include "gent_util.h"
#include "gent_level.h"
#include "gent_db.h"
#include "gent_list.h"
#include "gent_app_mgr.h"

GentLevel::GentLevel(GentConnect *c):GentCommand(c)
{
  keystr = "";
  remains = 0;
  max_tokens = 1000;
}
GentLevel::~GentLevel()
{}

uint8_t GentLevel::Split(const string &str, const string &delimit, vector<string> &v) {
    uint64_t pos,last_pos=0;
	uint8_t num = 0;
    while((pos = str.find_first_of(delimit,last_pos)) != string::npos){
		if(pos == last_pos){
            last_pos++;
        }else{
            v.push_back(str.substr(last_pos, pos-last_pos));
			num++;
            last_pos = pos+1;
        }
    }
	size_t ter_pos = str.find_first_of("\r\n",last_pos);
	if(ter_pos == string::npos) {
		return 0;
	}
	
    if(ter_pos>last_pos){
        string curstr = str.substr(last_pos, ter_pos-last_pos);
		v.push_back(curstr);
		num++;
    }
    /*
    vector<string>::iterator iter;
    for(iter=v.begin(); iter!=v.end();iter++){
        cout<<"split:" <<  *iter << endl;
    }
    */
	return num;
}


int GentLevel::CommandWord() {	
	//vector<string> tokenList;
	tokenList.clear();
	uint8_t clength = Split(commandstr, " ", tokenList);
	LOG(GentLog::INFO, "tokenList clength: %d", clength);
	if(clength == 2 && tokenList[0] == "get") {
		LOG(GentLog::INFO, "tokenList get");
		commandtype = CommandType::COMM_GET;
        keystr = tokenList[1];
		conn->SetStatus(Status::CONN_DATA);
		return 0;
	}else if(clength > 2  && tokenList[0] == "get") {
        if(clength > max_tokens) {
            return -1;
        }
        keystr = "";
        keys.clear();
        for(vector<string>::size_type i=1; i!=tokenList.size(); i++) {
            keys.push_back(tokenList[i]);
        }
        LOG(GentLog::INFO, "tokenList multi get");
		commandtype = CommandType::COMM_GET;
		conn->SetStatus(Status::CONN_DATA);
        return 0;
    }else if(clength == 5 && tokenList[0] == "set") {
	    LOG(GentLog::INFO, "the command is set and the key is %s", tokenList[1].c_str());
        keystr = tokenList[1];
		commandtype = CommandType::COMM_SET;
        conn->SetStatus(Status::CONN_NREAD);
        int vlen;                                                                 
        if(!GentUtil::SafeStrtol(tokenList[4].c_str(), (int32_t *)&vlen)) {
            LOG(GentLog::WARN, "the length of set's command  is failed");
            return -1;                                                             
        }
		LOG(GentLog::INFO, "need read the lenth of client's data is %d,and the key is %s", vlen, tokenList[1].c_str());
        rlbytes = vlen+2;
        return rlbytes-remains;
        return rlbytes; 
	}else if(clength == 2 && tokenList[0] == "del") {
		LOG(GentLog::INFO, "the command is del and the key is %s", tokenList[1].c_str());
        commandtype = CommandType::COMM_DEL;
        keystr = tokenList[1];
        conn->SetStatus(Status::CONN_DATA);
        return 0; 
	}else if(clength == 1 && tokenList[0] == "quit") {
		LOG(GentLog::INFO, "the command is quit");
        commandtype = CommandType::COMM_QUIT;
        conn->SetStatus(Status::CONN_CLOSE);
        return 0; 
	}else if(clength ==2 && tokenList[0] == "stats") {
		LOG(GentLog::INFO, "the command is stats");
		commandtype = CommandType::COMM_STATS;
		keystr = tokenList[1];
		conn->SetStatus(Status::CONN_DATA);  
		return 0; 
	}else if(clength == 1 && tokenList[0] == "stats") {
        LOG(GentLog::INFO, "the command is stats");
        commandtype = CommandType::COMM_STATS;
		keystr = "";
        conn->SetStatus(Status::CONN_DATA);
        return 0;
    }
	return -1;
}


int GentLevel::ParseCommand(const string &str) {
	if(str.size() == 0) return 0;
	uint64_t pos = str.find_first_of("\n");
	if(pos == string::npos || pos == 0) return 0;
	commandstr = str.substr(0, pos);
	content = "";
	remains = str.size()-pos-1;
	content.assign(str.substr(pos+1,remains));
	return 1;
}

int GentLevel::Process(const char *rbuf, uint64_t size, string &outstr) {
	if(size <=0 ) {	
		outstr = "ERROR\r\n";
		return 0;
	}
	string data = string(rbuf, size);
	if(!ParseCommand(data)) {
		outstr = "ERROR\r\n";
		return 0;	
	}
	
	int cword = CommandWord();
    if(cword < 0) {
        outstr = "CLIENT ERROR\r\n";
        return 0;
    }
	if(cword == 0) return cword;
	return cword;
}
void GentLevel::ProcessDel(string &outstr)
{
	/*
	if(!GentList::Instance()->Load(keystr)) {
		outstr = "NOT_FOUND\r\n";
		return;
	}
	*/
	string nr = "";
	if(!GentDb::Instance()->Get(keystr, nr)){
		outstr = "NOT_FOUND\r\n";
		return;                  
	}
	//GentList::Instance()->Clear(keystr);			
	if(!GentDb::Instance()->Del(keystr)) {
		outstr = "NOT_FOUND\r\n";
	}else{
		outstr = "DELETED\r\n";
	}
}

void GentLevel::ProcessGet(string &outstr)
{
    if(keystr == "") {
        ProcessMultiGet(outstr);
        return;
    }
    string nr="";
    if(!GentDb::Instance()->Get(keystr, nr))
    {
        outstr = "END\r\n";
    }
    char retbuf[200]={0};
    snprintf(retbuf,200, "VALUE %s 0 %u\r\n",keystr.c_str(), (unsigned int)nr.size());
    outstr = retbuf;
	outstr += nr+"\r\nEND\r\n";
}

void GentLevel::ProcessMultiGet(string &outstr)
{
    vector<string>::iterator iter;
    for(iter=keys.begin(); iter!=keys.end(); iter++)
    {
        string nr="";
        if(!GentDb::Instance()->Get(*iter, nr))
        {
            continue;
        }
        char retbuf[200]={0};
        snprintf(retbuf,200, "VALUE %s 0 %u\r\n",(*iter).c_str(), (unsigned int)nr.size());
        outstr += retbuf;
        outstr += nr+"\r\n";
    }
    outstr += "END\r\n";
}

void GentLevel::ProcessStats(string &outstr)
{
    char retbuf[200] = {0};
	uint64_t num = GentDb::Instance()->Count(keystr);
    uint64_t totals = GentDb::Instance()->TotalSize();
	snprintf(retbuf,200,"total_connect: %u\r\ncurrent_connect: %u\r\nitem_nums: %llu\r\ntotal_size: %llu\r\n",
         (unsigned int)GentAppMgr::Instance()->GetTotalConnCount(),
		 (unsigned int)GentAppMgr::Instance()->GetConnCount(), 
		 (unsigned long long)num, (unsigned long long)totals); 
    outstr = retbuf;
}

void GentLevel::Complete(string &outstr, const char *recont, uint64_t len)
{
    char buf[20]={0};
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
			//NOT_STORED
			LOG(GentLog::WARN, "commandtype::comm_set");
			content += string(recont,len);
			if(content.substr(rlbytes-2,2)!="\r\n") {
				outstr = "CLIENT_ERROR bad data chunk\r\n";
				LOG(GentLog::WARN, "CLIENT_ERROR bad data chunk");
			}else{
				string nr;                   
 				nr.assign(content.c_str(), rlbytes-2);
				if(!GentDb::Instance()->Put(keystr, nr,0, 0)) {
                    outstr = "NOT_STORED\r\n";
                }else{
                    //GentList::Instance()->Save(keystr);
                    LOG(GentLog::WARN, "commandtype::comm_set stored");
                    sprintf(buf,"STORED\r\n");
                    //outstr = "STORED\r\n";
                    outstr.assign(buf,8);
                }
				//outstr = "STORED\r\n";
			}
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
GentCommand *GentLevel::Clone(GentConnect *connect)
{
	return new GentLevel(connect);
}
bool GentLevel::Init(string &msg)
{
   if(!GentDb::Instance()->Init(msg))
   {
       LOG(GentLog::ERROR, "db init fail,%s",msg.c_str());
       return false;
   }
   //GentList::Instance()->Init();
   return true;
}

void GentLevel::AssignVal(token_t *tokens)
{
    string tmp(tokens[1].value);
    keystr.assign(tmp,0,tmp.size());
    
}
