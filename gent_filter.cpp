
#include "prefine.h"
#include "gent_util.h"
#include "gent_db.h"
#include "gent_filter.h"
#include "gent_link.h"
#include "gent_frame.h"
#include "gent_app_mgr.h"
#include "gent_find.h"


GentFilter::GentFilter(GentConnect *c):GentCommand(c)
{
  keystr = "";
  remains = 0;
}
GentFilter::~GentFilter()
{}

uint8_t GentFilter::Split(const string &str, const string &delimit, vector<string> &v) {
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


int GentFilter::CommandWord() {	
	vector<string> tokenList;
	uint8_t clength = Split(commandstr, " ", tokenList);
	LOG(GentLog::INFO, "tokenList clength: %d", clength);
	if(clength == 2 && tokenList[0] == "get") {
		LOG(GentLog::INFO, "tokenList get");
		commandtype = CommandTypeFilter::COMM_GET;
        keystr = tokenList[1];
		conn->SetStatus(Status::CONN_DATA);
		return 0;
	}else if(clength == 5 && tokenList[0] == "set") {
	    LOG(GentLog::INFO, "the command is set and the key is %s", tokenList[1].c_str());
        keystr = tokenList[1];
		commandtype = CommandTypeFilter::COMM_SET;
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
        commandtype = CommandTypeFilter::COMM_DEL;
        keystr = tokenList[1];
        conn->SetStatus(Status::CONN_DATA);
        return 0; 
	}else if(clength == 1 && tokenList[0] == "quit") {
		LOG(GentLog::INFO, "the command is quit");
        commandtype = CommandTypeFilter::COMM_QUIT;
        conn->SetStatus(Status::CONN_CLOSE);
        return 0; 
	}else if(clength == 1 && tokenList[0] == "stats") {
        LOG(GentLog::INFO, "the command is stats");
        commandtype = CommandTypeFilter::COMM_STATS;
        conn->SetStatus(Status::CONN_DATA);
        return 0;
    }
	return -1;
}


int GentFilter::ParseCommand(const string &str) {
	if(str.size() == 0) return 0;
	uint64_t pos = str.find_first_of("\n");
	if(pos == string::npos || pos == 0) return 0;
	commandstr = str.substr(0, pos);
	content = "";
	remains = str.size()-pos-1;
	content.assign(str.substr(pos+1,remains));
	return 1;
}

int GentFilter::Process(const char *rbuf, uint64_t size, string &outstr) {
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

void GentFilter::ProcessGet(string &outstr)
{
    
    GentLink *link = GentLinkMgr::Instance()->GetLink(keystr);
    if(!link) {
        outstr = "END\r\n";
        return;
    }
    string curkey;
    if(!link->Pop(curkey)){
        outstr = "END\r\n";
        return;
    }
    
    string nr="";
    if(!GentDb::Instance()->Get(curkey, nr))
    {
        outstr = "END\r\n";
    }
    char retbuf[200]={0};
    snprintf(retbuf,200, "VALUE %s 0 %ld\r\n",keystr.c_str(), nr.size());
    outstr = retbuf;
	outstr += nr+"\r\nEND\r\n";
}

void GentFilter::ProcessStats(string &outstr)
{
    char retbuf[200] = {0};
    snprintf(retbuf,200,"total connect: %lu\r\ncurrent connect: %lu\r\n",
             GentAppMgr::Instance()->GetTotalConnCount(),GentAppMgr::Instance()->GetConnCount());
    outstr = retbuf;
}

void GentFilter::ProcessSet(string &outstr, string &cont)
{
   vector<string> v;
   GentFind gfind;
   gfind.Search(cont, v);
   outstr = "END\r\n"; 
}

void GentFilter::Complete(string &outstr, const char *recont, uint64_t len)
{
	switch(commandtype)
	{
		case CommandTypeFilter::COMM_GET:
			//NOT_FOUND
           ProcessGet(outstr);
			break;	
		case CommandTypeFilter::COMM_SET:
			//NOT_STORED
			LOG(GentLog::WARN, "commandtype::comm_set");
			content += string(recont,len);
			if(content.substr(rlbytes-2,2)!="\r\n") {
				outstr = "CLIENT_ERROR bad data chunk\r\n";
				LOG(GentLog::WARN, "CLIENT_ERROR bad data chunk");
			}else{
                ProcessSet(outstr, content);
			}
			break;
		case CommandTypeFilter::COMM_QUIT:
			break;
		case CommandTypeFilter::COMM_DEL:
			//clear queue
			outstr = "ERROR\r\n";
			break;
        case CommandTypeFilter::COMM_STATS:
            ProcessStats(outstr);
            break;
		default:
			outstr = "ERROR\r\n";
			return;
	}		

}
GentCommand *GentFilter::Clone(GentConnect *connect)
{
	return new GentFilter(connect);
}
bool GentFilter::Init(string &msg)
{
   GentFindMgr::Instance()->Init();
   return true;
}

void GentFilter::AssignVal(token_f *tokens)
{
    string tmp(tokens[1].value);
    keystr.assign(tmp,0,tmp.size());
    
}
