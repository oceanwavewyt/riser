/*
 * gent_frame.h
 *
 *  Created on: 2012-2-19
 *      Author: wyt
 */

#ifndef GENT_FRAME_H_
#define GENT_FRAME_H_
#include "prefine.h"
#include "gent_event.h"
#include "gent_msg.h"
#include "gent_config.h"
class GentBasic;

class GentFrame
{
	typedef std::map<int, GentBasic *> MODULE_MAP;
private:
	MODULE_MAP modules_;
	static GentFrame *instance_;
    
    struct sockaddr_in addr;
public:
	GENT_MSG_CONNECT msg_;
    GentConfig config;
	struct riserserver *s;
public:
	GentFrame();
	~GentFrame();
public:
	static GentFrame *Instance();
	static void Unstance();
private:
	int Socket();
	int ServerSocket(int port=-1);
public:
    int Init(struct riserserver *s, const char *configfile="riser.conf");
	int Run(int port);
	int Register(int key, GentBasic *app);
	void Destory();
	int GetModule(GentBasic *&app, int cmd);

};

#endif /* GENT_FRAME_H_ */
