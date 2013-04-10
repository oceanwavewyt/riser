
#include "prefine.h"
#include "gent_util.h"
#include "gent_level.h"
#include "gent_db.h"

GentLevel::GentLevel(GentConnect *c):GentCommand(c)
{
  keystr = "";
  remains = 0;
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
    if(str.size()!=last_pos){
        string curstr = str.substr(last_pos, str.find_first_of("\r\n",last_pos)-last_pos);
        v.push_back(curstr);
		num++;
    }
//    vector<string>::iterator iter;
//    for(iter=v.begin(); iter!=v.end();iter++){
//        cout<<"split:" <<  *iter << endl;
//    }
	return num;
}


int GentLevel::CommandWord() {	
	vector<string> tokenList;
	uint8_t clength = Split(commandstr, " ", tokenList);
	LOG(GentLog::INFO, "tokenList clength: %d", clength);
	if(clength == 2 && tokenList[0] == "get") {
		LOG(GentLog::INFO, "tokenList get");
		commandtype = CommandType::COMM_GET;
        keystr = tokenList[1];
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
        //AssignVal(token);
        keystr = tokenList[1];
        conn->SetStatus(Status::CONN_DATA);
        return 0; 
	}else if(clength == 1 && tokenList[0] == "quit") {
		LOG(GentLog::INFO, "the command is quit");
        commandtype = CommandType::COMM_QUIT;
        conn->SetStatus(Status::CONN_CLOSE);
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
void GentLevel::ProcessGet(string &outstr)
{
    string nr="";
    if(!GentDb::Instance()->Get(keystr, nr))
    {
        nr="NOT_FOUND\r\n";
    }
    char retbuf[200]={0};
    snprintf(retbuf,200, "VALUE %s 0 %ld\r\n",keystr.c_str(), nr.size());
	outstr = retbuf;
    outstr += nr+"\r\nEND\r\n";
}

void GentLevel::Complete(string &outstr, const char *recont, uint64_t len)
{
	switch(commandtype)
	{
		case CommandType::COMM_GET:
			//NOT_FOUND
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
				if(!GentDb::Instance()->Put(keystr, nr)) {
                    outstr = "NOT_STORED\r\n";
                }else{
                    LOG(GentLog::WARN, "commandtype::comm_set stored");
                    outstr = "STORED\r\n";
                }
				//outstr = "STORED\r\n";
			}
			break;
		case CommandType::COMM_QUIT:
			break;
		case CommandType::COMM_DEL:
            outstr = "STORED\r\n";
			break;
		default:
			outstr = "ERROR\r\n";
			return;
	}		

}

void GentLevel::AssignVal(token_t *tokens)
{
    string tmp(tokens[1].value);
    keystr.assign(tmp,0,tmp.size());
    
}
