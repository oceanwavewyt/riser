#include "gent_command.h"

GentCommand::GentCommand(GentConnect *c)
{
    conn = c;
    //Init();
}

GentCommand::~GentCommand()
{

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


