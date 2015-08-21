#ifndef riser_gent_repl_h
#define riser_gent_repl_h
#include "gent_queue_list.h"

class GentReplication
{
	string rep_name;
	//0:初始状态;1:正在同步状态;
	int status;
	//当全量同步时候，导出所有影片的ID到main_que
	bool is_update_main_que;
	//queue node point
	NODE<itemData*> *current_node;
	//主队列
	GentListQueue<itemData*> main_que;
	//在同步所有的数据时，不写如main_que,而写入que_
	GentListQueue<itemData*> que;
public:
	GentReplication(const string &name);
	~GentReplication();	
	bool Check();
	bool Start();
};


class GentRepMgr
{
	static GentRepMgr *intance_;
	std::map<string,GentReplication*> rep_list_;	
private:
	GentReplication *Get(string &name);
public:
	static GentRepMgr *Instance();
	static void UnInstance();
public:
	GentRepMgr();
	~GentRepMgr();
	void Destroy(int id);	
	bool Logout(string &name);	
	bool Run(string &name, string &runid);
};

#endif
