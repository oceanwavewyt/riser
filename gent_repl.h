#ifndef riser_gent_repl_h
#define riser_gent_repl_h
#include "gent_queue_list.h"
class GentEvent;
class GentConnect;
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
	bool Start(string &msg, string &outstr);
	void Push(int type, string &key);
private:
	void Reply(int type, string &key,string &outstr, const string &nr="");
};


class GentRepMgr
{
	static GentRepMgr *intanceMaster_;
	static GentRepMgr *intanceSlave_;
	std::map<string,GentReplication*> rep_list_;	
	GentConnect *connect_;
	int status;
private:
	GentReplication *Get(string &name);
	int LinkMaster(GentEvent *ev_, const string &host, int port);	
public:
	enum status {INIT=0,AUTH=1,TRAN=2,COMPLETE=3};
public:
	static GentRepMgr *Instance(const string &name);
	static void UnInstance();
	static void SlaveHandle(int fd, short which, void *arg);
public:
	GentRepMgr();
	~GentRepMgr();
	void Destroy(int id);	
	bool Logout(string &name);	
	bool Run(string &name,string &msg, string &outstr);
	void Push(int type, string &key);
	int SlaveAuth(GentEvent *ev_, const string &client_name);
	void SlaveReply(string &outstr, int suc);
	void SlaveSetStatus(int t);
	void Slave(GentEvent *e);
	void CannelConnect();
};

#endif
