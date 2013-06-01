//
//  gent_command.h
//  riser
//
//  Created by wyt on 13-1-27.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#ifndef riser_gent_event_h
#define riser_gent_event_h

#include "gent_command.h"
#include "gent_connect.h"

typedef struct token_sq {
    char *value; 
    size_t length;
} token_q;

class CommandTypeQueue
{
public:
	enum ct
	{
	 COMM_GET = 1,
 	 COMM_SET = 2,
 	 COMM_DEL = 3,
 	 COMM_QUIT = 4,
     COMM_STATS = 5,
	};

};


class GentQueue : public GentCommand
{
   uint64_t  rlbytes;
   uint32_t  remains;	
   
   uint8_t commandtype; 
   string keystr;
   string content;
   string commandstr;
public:
    GentQueue(GentConnect *c=NULL);
    ~GentQueue();
private:
   size_t TokenCommand(char *command, token_q *tokens, const size_t max_tokens);
   int ParseCommand(const string &str);
   uint8_t Split(const string &str, const string &delimit, vector<string> &v);
   int CommandWord();
   void AssignVal(token_q *tokens);
   void ProcessGet(string &);
   void ProcessStats(string &);
public:
   int Process(const char *rbuf, uint64_t size, string &outstr);	
   void Complete(string &outstr, const char *, uint64_t);
   GentCommand *Clone(GentConnect *);
   int GetStatus();
   bool Init(string &msg);
};


#endif
