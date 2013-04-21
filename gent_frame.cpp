/*
 * gent_frame.cpp
 *
 *  Created on: 2012-2-19
 *      Author: wyt
 */
#include "prefine.h"
#include "gent_event.h"
#include "gent_frame.h"
#include "gent_app_mgr.h"
#include "gent_thread.h"
#include "gent_level.h"

GentFrame *GentFrame::instance_ = NULL;

GentFrame *GentFrame::Instance() {
	if(NULL == instance_) {
		instance_ = new GentFrame();
	}
	return instance_;
}

void GentFrame::Unstance() {
	if(instance_ != NULL) {
		delete instance_;
	}
}

GentFrame::GentFrame() {
	LOG(GentLog::INFO, "gentframe init.");
	msg_.Resize(100);
}


GentFrame::~GentFrame() {
	Destory();
}

int GentFrame::Init(const char *configfile)
{
    if(access(configfile, 0) == -1) {
        LOG(GentLog::ERROR, "%s not exist, start fail.", configfile);
        return false;
    }
    config.Parse(string(configfile,strlen(configfile)));
    //config info
    string msg;
	GentLevel *p;
	REGISTER_COMMAND(p, GentLevel);
	if(!p->Init(msg))
    {
        return false;
    }
    return true;

}

int GentFrame::Socket() {
	int listenfd;
	int flags = 1;
	listenfd = socket(AF_INET,SOCK_STREAM,0);
	if(listenfd == -1) {
		LOG(GentLog::ERROR, "socket create failed.");	
		return -1;
	}
	if( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(int)) == -1) {
		LOG(GentLog::ERROR, "set socket option failed.");
		return -1;
	}
    setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
    
    
    struct linger so_linger={0,0};
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, (void *)&so_linger, sizeof(so_linger));
    setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));

    
	if ((flags = fcntl(listenfd, F_GETFL, 0)) < 0 ||
	        fcntl(listenfd, F_SETFL, flags | O_NONBLOCK) < 0) {
		LOG(GentLog::ERROR, "setting nonblock failed.");	
	    close(listenfd);
	    return -1;
	}
	return listenfd;
}

int GentFrame::ServerSocket(int port)  {
	int fd;
    if(port == -1) {
        port = atoi(config["port"].c_str());
    }
	if((fd = Socket()) == -1) {
		return -1;
	}
	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(fd,(struct sockaddr *)&addr,sizeof(addr)) == -1 ) {
		LOG(GentLog::ERROR, "bind port %d failed.", port);	
		close(fd);
		return -1;
	}
	if(listen(fd,1024) == -1) {
		LOG(GentLog::ERROR, "listen failed.");	
		close(fd);
		return -1;
	}
	LOG(GentLog::INFO, "bind port %d success.",port);	
	return fd;
}

int GentFrame::Run(int port) {
    int fd = ServerSocket(port);
	if(fd <= 0) {
		return -1;
	}
	GentConnect *conn = new GentConnect(fd);
    GentEvent *gevent = new GentEvent();
    conn->gevent = gevent;
    
	gevent->AddEvent(conn,GentEvent::HandleMain);
	GentThread::Intance()->init(atoi(config["thread"].c_str()));
	//启动线程
//    std::cout << "start " << std::endl;
	GentThread::Intance()->Start();
	gevent->Loop();
    return 0;
}

int GentFrame::Register(int key, GentBasic *app) {
	MODULE_MAP::iterator mit = modules_.find(key);
	if(mit != modules_.end()) {
		return 0;
	}
	modules_[key] = app;
	GentAppMgr::Instance()->Register(key,app);
	return 0;
}

void GentFrame::Destory() {
	MODULE_MAP::iterator it;
	for(it = modules_.begin();it!=modules_.end(); ++it) {
		//delete it->second;
		//it->second = NULL;
	}
}

int GentFrame::GetModule(GentBasic *&app, int cmd) {
	MODULE_MAP::iterator mit = modules_.find(cmd);
	if(mit == modules_.end()) {
		return -1;
	}
	app = modules_[cmd];
	return 0;
}

