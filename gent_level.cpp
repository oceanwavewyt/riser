
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
            cout << "last_pos1: " << last_pos << endl;
        }else{
            v.push_back(str.substr(last_pos, pos-last_pos));
			num++;
            last_pos = pos+1;
            cout << "last_pos2: " << last_pos << endl;
        }
    }
    cout << "str.size(): " << str.size() << "last_pos: " << last_pos << endl;
    if(str.size()!=last_pos){
        string curstr = str.substr(last_pos, str.find("\r\n",last_pos)-last_pos);
        cout<< "curstr: "<< endl;
        v.push_back(curstr);
		num++;
    }
    vector<string>::iterator iter;
    for(iter=v.begin(); iter!=v.end();iter++){
        cout<<"split: " <<  *iter << endl;
    }
	return num;
}
size_t GentLevel::TokenCommand(char *command, token_t *tokens, const size_t max_tokens)
{
    char *s, *e;                                                                                   
    size_t ntokens = 0;                                                                            
    size_t len = strlen(command);                                                                  
    unsigned int i = 0;                                                                            
    assert(command != NULL && tokens != NULL && max_tokens > 1);                                   
                                                                                                   
    s = e = command;                                                                               
    for (i = 0; i < len; i++) {                                                                    
        if (*e == ' ') {                                                                           
            if (s != e) {                                                                          
                tokens[ntokens].value = s;                                                         
                tokens[ntokens].length = e - s;                                                    
                //cout <<"value: "<< tokens[ntokens].value << endl;
                                                                                                   
                ntokens++;                                                                         
                *e = '\0';                                                                         
                if (ntokens == max_tokens - 1) {                                                   
                    e++;                                                                           
                    s = e; /* so we don't add an extra token */                                    
                    break;                                                                         
                }                                                                                  
            }                                                                                      
            s = e + 1;                                                                             
        }                                                                                          
        e++;                                                                                       
    }                                                                                              
                                                                                                   
    if (s != e) {                                                                                  
        tokens[ntokens].value = s;                                                                 
        tokens[ntokens].length = e - s;                                                            
        ntokens++;                                                                                 
    }                                                                                              
    /*                                                                      
     * If we scanned the whole string, the terminal value pointer is null,  
     * otherwise it is the first unprocessed character.                     
     */                                                                     
    tokens[ntokens].value =  *e == '\0' ? NULL : e;                         
    tokens[ntokens].length = 0;                                             
    ntokens++;                                                              
                                                                            
    return ntokens;                                                         
}

int GentLevel::CommandWord() {
   // int maxtoken = 10;                                                            
   // token_t token[maxtoken];                                                      
	
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

int GentLevel::ParseCommand() {
    char *el, *cont;                                     
                                                         
    if (rbytes == 0)                                  
        return 0;                                        
    el = (char *)memchr(rbuf, '\n', rbytes);       
    if (!el) {                                           
        return 0;                                        
    }                                                    
    cont = el + 1;                                       
    if ((el - rbuf) > 1 && *(el - 1) == '\r') {       
        el--;                                            
    }                                                    
    *el = '\0';                                          
    //cout <<"c->rcurr:" << c->rcurr << endl;            
    //assert(cont <= (c->rcurr + c->rbytes));            
                                                         
    //process_command(c, c->rcurr);                      
                                                         
    rbytes -= (cont - rcurr);                      
    rcurr = cont;                                     
    return 1;                                            
}                                                        

int GentLevel::ParseCommand2(const string &str) {
	if(str.size() == 0) return 0;
	uint64_t pos = str.find_first_of("\n");
	if(pos == string::npos) return 0;
	commandstr = str.substr(0, pos);
	content = "";
	remains = str.size()-pos;
	content.assign(str.substr(pos+1,remains));
	cout << "string commandStr: "<< commandstr <<endl;
	return 1;
}

int GentLevel::Process(const char *rbuf, uint64_t size, string &outstr) {
	// rcurr =  rbuf;
	if(size <=0 ) {	
		outstr = "ERROR\r\n";
		return 0;
	}
	string data = string(rbuf, size);
	if(!ParseCommand2(data)) {
		outstr = "ERROR\r\n";
		return 0;	
	}
	
	//rbytes = blen;
	//if(!ParseCommand()) {
	//	outstr = "ERROR\r\n";
	//	return 0;	
	//}
	int cword = CommandWord();
    if(cword < 0) {
        outstr = "CLIENT ERROR\r\n";
        return 0;
    }
	if(cword == 0) return cword;
	return cword;
//	int norbytes = cword - rbytes;
//	return norbytes;		
//	if(norbytes > 0) return norbytes; 	    
//	return cword;	
}
void GentLevel::ProcessGet(string &outstr)
{
    string nr="";
    if(!GentDb::Instance()->Get(keystr, nr))
    {
        nr="NOT_FOUND\r\n";
    }
    char retbuf[100];
    sprintf(retbuf,"VALUE %s 0 %ld\r\n",keystr.c_str(),nr.size());
    outstr = retbuf;
    outstr += nr+"\r\nEND\r\n";
}

void GentLevel::Complete(string &outstr, const char *recont)
{
	switch(commandtype)
	{
		case CommandType::COMM_GET:
			//NOT_FOUND
            		ProcessGet(outstr);
			break;	
		case CommandType::COMM_SET:
			//NOT_STORED
			//cout << "content1:  " << content << endl;
			LOG(GentLog::WARN, "commandtype::comm_set");
            //cout << "recont:  " << recont << endl;
			//content += recont;
			cout << "content:  " << content << endl;
			if(content.substr(rlbytes-2,2)!="\r\n") {
				outstr = "CLIENT_ERROR bad data chunk\r\n";
			}else{
				outstr = "STORED\r\n";
			}
		
/*	
			break;
	
			if (strncmp(content+rlbytes-2, "\r\n", 2) != 0) {
				outstr = "CLIENT_ERROR bad data chunk\r\n";
			}else{
                string nr;
                nr.assign(content,rlbytes-2);
                if(!GentDb::Instance()->Put(keystr, nr))
                {
                    outstr = "NOT_STORED\r\n";
                }else{
                    LOG(GentLog::WARN, "commandtype::comm_set stored");
                    outstr = "STORED\r\n";
                }
			}
*/
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
