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
class GentCommand;
typedef std::map<int,std::vector<GentBasic *> > APP_MODULE;
typedef std::map<int,GentCommand*> PLUGIN;

class GentAppMgr
{
	static GentAppMgr *intance_;
private:
	APP_MODULE app_mgr_;
	PLUGIN plus_mgr;
	unsigned int def_num;
	GentCommand *plus;
public:
	static GentAppMgr *Instance();
	static void UnInstance();
public:
	GentAppMgr();
	~GentAppMgr();
	int Register(int cmd, GentBasic *app);
	int GetModule(int cmd,GentBasic *&);
	int SetModule(int cmd, GentBasic *&app);
	
	bool Init();
	void SetPlugin(GentCommand *command);	
	GentCommand *GetCommand(int id);
	void Destroy(int id);		
};


#endif /* GENT_APP_MGR_H_ */
