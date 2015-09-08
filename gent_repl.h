#ifndef riser_gent_repl_h
#define riser_gent_repl_h
#include "gent_queue_list.h"
#include "gent_file.h"

class GentEvent;
class GentConnect;

typedef struct repinfo
{
    char name[SLAVE_NAME_SIZE];
	uint32_t name_len;
	uint8_t  available;
    uint64_t rep_time;
    uint64_t ser_time;
	void set(const string &n) {
		memcpy(name, n.c_str(), n.size());
		name_len = n.size();
		available = 1;
	};
}repinfo;

class GentReplication
{
	string rep_name;
	//0:初始状态;1:正在同步状态;
	int status;
	//当全量同步时候，导出所有影片的ID到que
	bool is_update_que;
	//queue node point
	NODE<itemData*> *current_node;
	//主队列
	GentListQueue<itemData*> main_que;
	//在同步所有的数据时，不写如main_que,而写入que_
	GentListQueue<itemData*> que;
	repinfo *rinfo_;
	CommLock que_push_lock;
	CommLock que_pop_lock;
public:
	GentReplication(const string &name, repinfo *rinfo);
	~GentReplication();	
	bool Start(string &msg, string &outstr);
	void Push(int type, string &key);
private:
	void Reply(int type, string &key,string &outstr, const string &nr="");
	void Pop();	
	itemData *front_element();
};


class GentRepMgr
{
	static GentRepMgr *intanceMaster_;
	static GentRepMgr *intanceSlave_;
	std::map<string,GentReplication*> rep_list_;	
	GentConnect *connect_;
	int status;
	GentFile<repinfo> *repinfo_;
	map<string,repinfo *> rep_map_;
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
	GentRepMgr(const string &name);
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
