/*
 *  riser.cpp  
 *  Created by oceanwavewyt on 13-1-20.
 *  Copyright (c) 2012-2013, oceanwavewyt <oceanwavewyt at gmail dot com>
 *  All rights reserved.
 *  
 *  This soft wrap a serice for leveldb store, and support multi-threading 
 *  client access it by the memcache extension of php, and telnet. Currently,
 *  only support put, get and del operation. additionally, it is support bloom 
 *  filter algorithm for key.
 *
 */
#include "prefine.h"
#include "gent_util.h"
#include <queue>
#include <linenoise.h>
#include <hiredis.h>
#include <sds.h>
#include <net.h>


int port = 3555;
char host[100] = "127.0.0.1";

class GentRedisClient  
{  
public:  
    GentRedisClient(string ip, int port, int timeout = 2000);  
    virtual ~GentRedisClient();  
  
    bool ExecuteCmd(const char *cmd, size_t len, string &response);  
    redisReply* ExecuteCmd(const char *cmd, size_t len);  
  
private:  
    int m_timeout;  
    int m_serverPort;  
    string m_setverIp;  
    CommLock m_lock;
	std::queue<redisContext *> m_clients;  
  
    time_t m_beginInvalidTime;  
    static const int m_maxReconnectInterval = 3;  
  
    redisContext* CreateContext();  
    void ReleaseContext(redisContext *ctx, bool active);  
    bool CheckStatus(redisContext *ctx);  
};  
  
GentRedisClient::GentRedisClient(string ip, int port, int timeout)  
{  
    m_timeout = timeout;  
    m_serverPort = port;  
    m_setverIp = ip;  
  
    m_beginInvalidTime = 0;  
}  
  
GentRedisClient::~GentRedisClient()  
{  
	AutoLock lock(&m_lock); 
    while(!m_clients.empty())  
    {  
        redisContext *ctx = m_clients.front();  
        redisFree(ctx);  
        m_clients.pop();  
    }  
}  
  
bool GentRedisClient::ExecuteCmd(const char *cmd, size_t len,string &response)  
{  
    redisReply *reply = ExecuteCmd(cmd, len);  
    if(reply == NULL) return false; 
    if(reply->type == REDIS_REPLY_INTEGER)  
    { 
		char c[20]={0};
		snprintf(c,20,"%lld",reply->integer); 
        response = c;  
        return true;  
    }  
    else if(reply->type == REDIS_REPLY_STRING)  
    {  
        response.assign(reply->str, reply->len);  
        return true;  
    }  
    else if(reply->type == REDIS_REPLY_STATUS)  
    {  
        response.assign(reply->str, reply->len);  
        return true;  
    }  
    else if(reply->type == REDIS_REPLY_NIL)  
    {  
        response = "(nil)";  
        return true;  
    }  
    else if(reply->type == REDIS_REPLY_ERROR)  
    {
        response.assign(reply->str, reply->len); 
        return true;  
    }  
    else if(reply->type == REDIS_REPLY_ARRAY)  
    {  
		size_t i;
        for(i=0; i<reply->elements; i++) {
			redisReply *r =(redisReply *)reply->element[i];
			if(r->type == REDIS_REPLY_STRING) {
				cout << i<<") \""<< r->str << "\""<<endl;
			}
		} 
        return true;  
    }  
    else  
    {  
        response = "Undefine Reply Type";  
        return true;  
    }  
}  
  
redisReply* GentRedisClient::ExecuteCmd(const char *cmd, size_t len)  
{  
    redisContext *ctx = CreateContext();  
    if(ctx == NULL) return NULL;  
    redisReply *reply = (redisReply*)redisCommand(ctx,cmd);  
  	ReleaseContext(ctx, reply != NULL);  
  
    return reply;  
}  
  
redisContext* GentRedisClient::CreateContext()  
{  
    {  
		AutoLock lock(&m_lock); 
        if(!m_clients.empty())  
        {  
            redisContext *ctx = m_clients.front();  
            m_clients.pop();  
  
            return ctx;  
        }  
    }  
    time_t now = time(NULL);  
    if(now < m_beginInvalidTime + m_maxReconnectInterval) return NULL;  
  
    struct timeval tv;  
    tv.tv_sec = m_timeout / 1000;  
    tv.tv_usec = (m_timeout % 1000) * 1000;;  
    redisContext *ctx = redisConnectWithTimeout(m_setverIp.c_str(), m_serverPort, tv);  
	if(ctx == NULL || ctx->err != 0)  
    { 
        if(ctx != NULL) redisFree(ctx);  
 		 
        m_beginInvalidTime = time(NULL);  
      	cout << "connect failed" <<endl;
        return NULL;  
    }  
  
    return ctx;  
}  
  
void GentRedisClient::ReleaseContext(redisContext *ctx, bool active)  
{  
    if(ctx == NULL) return;  
    if(!active) {redisFree(ctx); return;}  
  
	AutoLock lock(&m_lock); 
    m_clients.push(ctx);  
}  
  
bool GentRedisClient::CheckStatus(redisContext *ctx)  
{  
    redisReply *reply = (redisReply*)redisCommand(ctx, "ping");  
    if(reply == NULL) return false;  
    if(reply->type != REDIS_REPLY_STATUS) return false;  
    if(strcasecmp(reply->str,"PONG") != 0) return false;  
  
    return true;  
}  

void prompt(const char *h, int p)
{
}

int main(int argc, char **argv)
{
    int ch;    
	while((ch = getopt(argc,argv,"h:vdsp:"))!= -1) {
        switch (ch) {
			case 'h':
				memcpy(host, optarg, strlen(optarg));					
				break;
            case 'p':
                port = atoi(optarg);
                break;
            default:
                break;
        }
    
    }
	string h = host;
	GentRedisClient client(h, port);	
	char prompt[200]={0};
	char historypath[200]={0};
	snprintf(prompt,200,"%s:%d> ",host,port);
	char *home = getenv("HOME");
	int history = 0;
	if (home != NULL && *home != '\0') {
		snprintf(historypath,200, "%s/.riserhistory",home);
		linenoiseHistoryLoad(historypath);
		history = 1;
	}
	char *line;
	while((line = linenoise(prompt)) != NULL) {
		if (line[0] != '\0') {	
			string data = line;	
			string res;
			data = GentUtil::Trim(data);
			if (history){
				linenoiseHistoryAdd(line);
            	linenoiseHistorySave(historypath);
			}
			free(line);
			if(client.ExecuteCmd(data.c_str(), data.size(),res)) {
				cout << res <<endl;
			}else{
				break;
			}
		}
	}
	return 0;	
}

