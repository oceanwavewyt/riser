//
//  main.cpp
//  riser
//
//  Created by wyt on 13-1-20.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#include "gent_util.h"
#include "gent_frame.h"
#include "gent_config.h"

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
    fprintf(stderr,"Usage: ./riser [/path/to/redis.conf] [options]\n");
    fprintf(stderr,"       ./riser - (read config from stdin)\n");
    fprintf(stderr,"       ./redis-server -v or --version\n");
    fprintf(stderr,"       ./redis-server -h or --help\n");
    fprintf(stderr,"       ./redis-server --test-memory <megabytes>\n\n");
    fprintf(stderr,"Examples:\n");
    fprintf(stderr,"       ./redis-server (run the server with default conf)\n");
    fprintf(stderr,"       ./redis-server /etc/redis/6379.conf\n");
    fprintf(stderr,"       ./redis-server --port 7777\n");
    fprintf(stderr,"       ./redis-server --port 7777 --slaveof 127.0.0.1 8888\n");
    fprintf(stderr,"       ./redis-server /etc/myredis.conf --loglevel verbose\n\n");
    fprintf(stderr,"Sentinel mode:\n");
    fprintf(stderr,"       ./redis-server /etc/sentinel.conf --sentinel\n");
    exit(1);
}


int main(int argc, const char * argv[])
{

	if (argc >= 2) {
    	/* Handle special options --help and --version */
   		if (strcmp(argv[1], "-v") == 0 ||
        	strcmp(argv[1], "--version") == 0) version();
    	if (strcmp(argv[1], "--help") == 0 ||
        	strcmp(argv[1], "-h") == 0) usage();
	}
    //char buf[100];
    //sprintf(buf,"get  abc  ");
    //Split(string(buf,strlen(buf))," ");
    //return 1;
    // insert code here...
    std::cout << "Hello, World!\n";
    //GentFrame::Instance()->Run(atoi(argv[1]));
    if(!GentFrame::Instance()->Init()){
        std::cout << "init fail!\n";
        return 1;
    }
    std::cout << GentFrame::Instance()->config["abc"] << endl;
    GentFrame::Instance()->config["abc"] = "test";
     std::cout << GentFrame::Instance()->config["abc"] << endl;
    GentFrame::Instance()->Run(10);
    return 0;
}

