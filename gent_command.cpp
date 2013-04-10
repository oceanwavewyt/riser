#include "gent_command.h"

GentCommand::GentCommand(GentConnect *c)
{
    conn = c;
    Init();
}

GentCommand::~GentCommand()
{

}

void GentCommand::Init()
{
    rsize = GentCommand::READ_BUFFER_SIZE;
    rcurr = NULL;
    rbuf = (char *)malloc(rsize);
    memset(rbuf,0,rsize);
}

void GentCommand::Reset()
{
/*
    if(rbuf) free(rbuf);
	if(content) free(content);
	content = NULL;
    Init();
*/
    //if(rcurr) free(rcurr);
}


