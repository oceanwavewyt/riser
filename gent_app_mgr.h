/*
 * gent_app_mgr.h
 *
 *  Created on: 2012-7-7
 *      Author: wyt
 */

#ifndef GENT_APP_MGR_H_
#define GENT_APP_MGR_H_
#include "prefine.h"
class GentBasic;
typedef map<int,std::vector<GentBasic *> > APP_MODULE;
class GentAppMgr
{
	static GentAppMgr *intance_;
private:
	APP_MODULE app_mgr_;
	unsigned int def_num;
public:
	static GentAppMgr *Intance();
	static void UnIntance();
public:
	GentAppMgr();
	~GentAppMgr();
	int Register(int cmd, GentBasic *app);
	int GetModule(int cmd,GentBasic *&);
	int SetModule(int cmd, GentBasic *&app);
};

#endif /* GENT_APP_MGR_H_ */
