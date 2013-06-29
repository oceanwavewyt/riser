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

#include "gent_util.h"
#include "gent_frame.h"
#include "gent_config.h"
#include "gent_link.h"
#include <sys/resource.h>


void daemonize(void) {
    int fd;

    if (fork() != 0) exit(0); /* parent exits */
    setsid(); /* create a new session */

    /* Every output goes to /dev/null. If Redis is daemonized but
 *      * the 'logfile' is set to 'stdout' in the configuration file
 *           * it will not log at all. */
    if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) close(fd);
    }
}

void version() {
    printf("1.0\n");
    exit(0);
}

void usage() {
    fprintf(stderr,"Usage: ./riser [options]\n");
    fprintf(stderr,"       ./riser -v or --version\n");
    fprintf(stderr,"       ./riser -h or --help\n");
    fprintf(stderr,"Examples:\n");
    fprintf(stderr,"       ./riser (run the server with default conf)\n");
    fprintf(stderr,"       ./riser -c /etc/riser.conf\n");
    fprintf(stderr,"       ./riser -p 3555\n");
    exit(1);
}


int main(int argc, char **argv)
{
	/*
	GentLinkMgr::Instance()->Init();	
	GentLink *link = GentLinkMgr::Instance()->GetLink("abc");
	if(!link) {
		cout << "not exist abc. " << endl;
		return 0;
	}
	link->Push("");
	string key;
	link->Pop(key);
	return 1;	
	*/
    int ch;
    bool deamon = false;
    int port = -1;
    char configfile[100] = "riser.conf";
    struct rlimit rlim;
	while((ch = getopt(argc,argv,"c:vhdp:"))!= -1) {
        switch (ch) {
            case 'c':
                printf("option a:'%s'\n",optarg);
                memcpy(configfile, optarg, strlen(optarg));
                break;
            case 'd':
                deamon = true;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'v':
                version();
                return 1;
            case 'h':
                usage();
                return 1;
            default:
                break;
        }
    
    }
    std::cout << "Hello, World!\n";
   	
	if(getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
		std::cout << "get rlimit failed\n";
		return 1;
	}
    /*
	rlim.rlim_cur = 65535;
	rlim.rlim_max = 65535;
	if(setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
		std::cout << "Set rlimit failed, please try starting as root\n";
		return 1;
	}
     */
	/* initialize config file */
	if(!GentFrame::Instance()->Init(configfile)){
        std::cout << "init fail!\n";
        return 1;
    }
    if(deamon == true) {
        daemonize();
    }
    GentFrame::Instance()->Run(port);
    return 0;
}

