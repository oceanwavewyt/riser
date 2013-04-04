
#include "prefine.h"
#include "gent_util.h"
#include "gent_level.h"
#include "gent_db.h"

GentLevel::GentLevel(GentConnect *c):GentCommand(c)
{
  keystr = "";
}
GentLevel::~GentLevel()
{}

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
    int maxtoken = 10;                                                            
    token_t token[maxtoken];                                                      
                                                                                  
    uint8_t ntokens = TokenCommand(rbuf, token, maxtoken);
    LOG(GentLog::INFO, "the number of tokens is %d.", ntokens);
    if(ntokens == 3 && memcmp(token[0].value, "get",3) == 0) {
        LOG(GentLog::INFO, "the command is get and the key is %s", token[1].value);
		commandtype = CommandType::COMM_GET;
        AssignVal(token);
        conn->SetStatus(Status::CONN_DATA);
        return 0;                                                                 
    }else if(ntokens == 6 && memcmp(token[0].value, "set",3) == 0) {
		LOG(GentLog::INFO, "the command is set and the key is %s", token[1].value);
        commandtype = CommandType::COMM_SET;
        AssignVal(token);
        conn->SetStatus(Status::CONN_NREAD);
        int vlen;                                                                 
        if(!GentUtil::SafeStrtol(token[4].value, (int32_t *)&vlen)) {
            LOG(GentLog::WARN, "the length of set's command  is failed");
            return -1;                                                             
        }
		LOG(GentLog::INFO, "need read the lenth of client's data is %d,and the key is %s", vlen, token[1].value);
        rlbytes = vlen+2;
        return rlbytes;                                           
    }else if(ntokens == 3 && memcmp(token[0].value, "del",3) == 0) {
		LOG(GentLog::INFO, "the command is del and the key is %s", token[1].value);
        commandtype = CommandType::COMM_DEL;
        AssignVal(token);
        conn->SetStatus(Status::CONN_DATA);
        return 0;                                           
    }else if(ntokens == 2 && memcmp(token[0].value, "quit",4) == 0) {
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

int GentLevel::Process(string &outstr) {
	 rcurr =  rbuf;
	//rbytes = blen;
	if(!ParseCommand()) {
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

void GentLevel::Complete(string &outstr)
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
