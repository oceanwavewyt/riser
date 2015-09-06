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
#include <sys/resource.h>

#define PATHBUF 100

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

bool getpath(char *filepath) {
	char *c = filepath;
	if(*c == '/') return true;
	char buf[PATHBUF] ={0};
	if(!getcwd(buf,PATHBUF)) return false;
	if(*c=='.') {
		c++;
		if(*c == '/') c++;
		else
		  c--;
	}
	char tt[PATHBUF]={0};
	sprintf(tt, "%s/%s", buf,c);
	memmove(filepath,tt,PATHBUF);
	return true;		
}

int main(int argc, char **argv)
{
    int ch;
    bool deamon = false;
	bool is_slave = false;
    int port = -1;
    char configfile[PATHBUF] = "riser.conf";
    struct rlimit rlim;
	while((ch = getopt(argc,argv,"c:vhdsp:"))!= -1) {
        switch (ch) {
            case 'c':
                printf("option a:'%s'\n",optarg);
                memcpy(configfile, optarg, strlen(optarg));
                break;
            case 'd':
                deamon = true;
                break;
			case 's':
				is_slave = true;
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
	getpath(configfile);
	struct riserserver server;
	server.configfile = configfile;
	/* initialize config file */
	if(!GentFrame::Instance()->Init(&server, configfile)){
        std::cout << "init fail!\n";
        return 1;
    }
    if(deamon == true) {
        daemonize();
    }
    GentFrame::Instance()->Run(port);
    return 0;
}

