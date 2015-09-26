#ifndef riser_gent_repl_h
#define riser_gent_repl_h
#include "gent_queue_list.h"
#include "gent_file.h"

class GentEvent;
class GentConnect;

class pageitem
{
public:
	uint32_t  available;
protected:
	char *head;
public:
	pageitem(){};
	pageitem(char *str){head = str;};
	virtual ~pageitem(){};
	virtual size_t size()=0;
	virtual void set(const string &str)=0;	
};

class repinfo : public pageitem
{
public:
    char name[SLAVE_NAME_SIZE];
    uint32_t rep_time;
    uint32_t ser_time;
	repinfo(){};
	repinfo(char *str):pageitem(str) {
		init(str);	
	};
	~repinfo(){};
    void set(const string &nstr) {
		string n = nstr;
		if((int)n.size()<SLAVE_NAME_SIZE) {
			size_t len = SLAVE_NAME_SIZE - n.size();
			string tmp= "";
			for(size_t i=0;i<len; i++) {
				tmp+="\x00";
			}
			n+=tmp;	
		}
        memcpy(name, n.c_str(), SLAVE_NAME_SIZE);
        available = 1;
        rep_time = 0;
        ser_time = 0;
		string wstr;	
		tostring(wstr);	
		memcpy(head, wstr.c_str(), wstr.size());
    };
	void set(const string &fieldname, uint64_t val)
	{
		int offset = 0;
		int ln = 0;
		if(fieldname == "ser_time") {
			ser_time = val;
			offset = 1;
			ln = 10;
		}else if(fieldname == "rep_time") {
			rep_time = val;
			offset = 11;
			ln = 10;
		}else if(fieldname == "available") {
			available = val;
			offset = 0;
			ln = 1;
		}
		char buf[12] = {0};
		snprintf(buf,ln,"%llu",(unsigned long long)val);
		memcpy(head+offset, buf, ln);
	}; 
	void tostring(string &str) {
		char buf[100] = {0};
		char st[12] = {0};
		if(ser_time == 0){
			memcpy(st,"0000000000",10);
		}else{
			snprintf(st,12,"%ld",(unsigned long)ser_time);	
		}
		char rt[12] = {0};
		if(rep_time == 0){
			memcpy(rt,"0000000000",10);
		}else{
			snprintf(rt,12,"%ld",(unsigned long)rep_time);	
		}
		snprintf(buf,100,"%ld%s%s%s",(unsigned long)available,st,rt,name);	
		cout << buf<<endl;
		str = buf;
	};
	size_t size() {
		return SLAVE_NAME_SIZE+21;
	};
	void init(char *str) {
		char t[2] ={0};
		memcpy(t,str,1);
		available = atoi(t);
		char tt[11];
		memcpy(tt,str+1,10);
		ser_time = atoi(tt);
		memcpy(tt,str+11,10);
		rep_time = atoi(tt);
		memcpy(name,str+21,SLAVE_NAME_SIZE);	
	};
};


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
	//队列长度
	uint64_t main_que_length;
	repinfo *rinfo_;
	uint64_t slave_start_time;
	uint64_t slave_last_time;
	GentConnect *conn_;
	CommLock que_push_lock;
	CommLock que_pop_lock;
public:
	GentReplication(const string &name, repinfo *rinfo);
	~GentReplication();	
	bool Start(string &msg, GentConnect *c, string &outstr);
	void Push(int type, string &key);
	uint64_t QueLength(){return main_que_length;};
	void GetInfo(string &str);
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
	GentFile<repinfo> *repfile_;
	map<string,repinfo *> rep_map_;
	uint64_t slave_start_time;
	string server_id_;
private:
	int LinkMaster(GentEvent *ev_, const string &host, int port);	
public:
	enum status {INIT=0,AUTH=1,WAIT=2,CONTINUE=3};
public:
	static GentRepMgr *Instance(const string &name);
	static void UnInstance();
	static void SlaveHandle(int fd, short which, void *arg);
public:
	GentRepMgr(const string &name);
	~GentRepMgr();
	void Destroy(int id);	
	bool Logout(string &name);	
	GentReplication *Get(const string &name);
	void Push(int type, string &key);
	uint32_t GetReplicationNum();
	uint64_t QueLength();
	void GetSlaveInfo(string &str);

	int SlaveAuth(const string &client_name, const string &authstr);
	void SlaveReply(string &outstr, int suc);
	void SlaveSetStatus(int t);
	void Slave(GentEvent *e);
	void CannelConnect();
	void GetInfo(string &str);
};

#endif
