//
//  gent_command.h
//  riser
//
//  Created by wyt on 13-1-27.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#ifndef riser_gent_level_h
#define riser_gent_level_h

#include "gent_command.h"
#include "gent_connect.h"

typedef struct token_s {
    char *value; 
    size_t length;
} token_t;

class CommandType
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


class GentLevel : public GentCommand
{
  // char   *rcurr;  /** but if we parsed some already, this is where we stopped */
   
//   char   *wbuf;
//   char   *wcurr;
//   int    wsize;
//   int    wbytes;
   /** which state to go into after finishing current write */
   //void   *write_and_free; /** free this memory after finishing writing */
   
//   char   *ritem;  /** when we read in an item's value, it goes here */
   uint64_t  rlbytes;
   uint32_t  remains;	
   
   uint8_t commandtype; 
   string keystr;
   string content;
   string commandstr;
   /*the command length of get is max_tokens*/
   int max_tokens;
    /*store multi key for get*/
   vector<string> keys;
public:
    GentLevel(GentConnect *c=NULL);
    ~GentLevel();
private:
   size_t TokenCommand(char *command, token_t *tokens, const size_t max_tokens);
   int ParseCommand(const string &str);
   uint8_t Split(const string &str, const string &delimit, vector<string> &v);
   int CommandWord();
   void AssignVal(token_t *tokens);
   void ProcessGet(string &);
   void ProcessMultiGet(string &);
   void ProcessDel(string &); 
   void ProcessStats(string &);
public:
   int Process(const char *rbuf, uint64_t size, string &outstr);	
   void Complete(string &outstr, const char *, uint64_t);
   GentCommand *Clone(GentConnect *);
   int GetStatus();
   bool Init(string &msg);
};


#endif
