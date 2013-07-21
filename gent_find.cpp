//
//  gent_list.cpp
//  bloom filter for riser
//  This soft wrap a serice for leveldb store, and support multi-threading    
//  client access it by the memcache extension of php, and telnet. Currently,
//  only support put, get and del operation. additionally, it is support bloom
//  filter algorithm for key.
//
//  Created by wyt on 13-5-6.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#include "gent_find.h"
#include "gent_config.h"
#include "gent_frame.h"
#include "gent_app_mgr.h"
#include "gent_util.h"
#include <fstream>

GentFindMgr *GentFindMgr::intance_ = NULL;

GentFindMgr *GentFindMgr::Instance() {
	if(intance_ == NULL) {
		intance_ = new GentFindMgr();
	}
	return intance_;
}

GentFindMgr::GentFindMgr():nodestable(0),
	length(1000){
}
GentFindMgr::~GentFindMgr(){
}

void *GentFindUtil::Gmalloc(size_t size) {    
	void *p;
	p = malloc(size);             
	if(!p) {
		printf("malloc failed\n");
		return 0;                 
	}
	memset(p,0,size);
	return p;
}   

void *GentFindUtil::Gcalloc(size_t size,int len) {
    void *p;
    p = calloc(size,len);
    if(!p) {
        printf("calloc failed\n");
        exit(0);
    }
    return p;
}

void GentFindUtil::Gfree(void *p) {
    if(p) {
        free(p);
    }
}

int GentFindUtil::Charwchar(char *str,wchar_t *out) {
	int ret = mbstowcs(out,str,strlen(str));
	if(ret == -1) {
		printf("charwchar failed \n");  
	}
	return ret;
}   

size_t GentFindUtil::Len(wchar_t *str) {
	return wcslen(str);
}   

size_t GentFindUtil::Wcstombs(char *buf,int buf_size,wchar_t *str) {
	size_t ntotal = wcstombs(buf,str,buf_size);
	if(ntotal == 0) {
		printf("wcstombs failed \n");                           
	}
	*(buf+buf_size-1)='\0';
	return ntotal;                                              
}       


node *GentFindMgr::NodeSet(int base,int check,int account,const char *name,short is_word) { 
	node *it = (node *)GentFindUtil::Gcalloc(1,sizeof(node));                                      
	it->base = base;                                                                   
	it->check = check;                                                                 
	it->child_count = account;                                                         
	it->is_word = is_word;                                                             
	strcpy(it->name,name);                                                             
	return it;                                                                         
}                                                                                      

void GentFindMgr::AddExQueue(int index, int childIndex) {
    nodestats_t::iterator it;
    it = nodestats.find(index);
    if(it == nodestats.end()) {
        std::vector<int> v;
        v.push_back(childIndex);
        nodestats[index] = v;
    }else {
        nodestats[index].push_back(childIndex);
    }
}

void GentFindMgr::AddExQueue(int index, std::vector<int> &vt) {
    if(vt.size()==0) return;
    nodestats[index] = vt;
}

void GentFindMgr::DelExQueue(int index) {
    nodestats_t::iterator it;
    it = nodestats.find(index);
    if(it == nodestats.end()) return;
    nodestats.erase(it);
    /*
    for(size_t k =0; k<nodestats[index].size();k++){
        cout << "del: "<< nodestats[index][k] << endl;
    }
    */
    /*
    queue_t *q = nodestats.head_ex;                                                  
    queue_t *pre_q = 0;                                                              
    do{                                                                              
        if(q->index != index){                                                       
            pre_q = q;                                                               
            q = q->next;                                                             
        }else {                                                                      
            queue_t *qu = q->next;                                                   
            if(pre_q == 0) {                                                         
                Gfree(nodestats.head_ex);                                          
                nodestats.head_ex = qu;                                              
            }else{                                                                   
                Gfree(pre_q->next);                                                
                pre_q->next = qu;                                                    
            }                                                                        
            break;                                                                   
        }                                                                            
    }while (q); 
    */
}                                                                                    
                                                                                     
long GentFindMgr::GetEncode(const char *key, int base_val, int is_asc)                    
{                                                                                    
    if(is_asc == 1){                                                                 
        return base_val+(key[0]);                                                    
    }else{                                                                           
        return base_val+(uint8_t)key[0]*256*256+(uint8_t)key[1]*256+(uint8_t)key[2]; 
    }                                                                                
}                                                                                    


void GentFindMgr::Init() {
	cout << "GentFindMgr::Init" << endl;
    
	nodestable = (node**)GentFindUtil::Gmalloc(length*sizeof(node *));
	memset(nodestable,0,length*sizeof(node *));   
	nodestable[0] = NodeSet(1,1,0,"",0);
    AddExQueue(0, 1);
	//nodestats.head_ex = NULL;
    
    setlocale(LC_ALL, "zh_CN.UTF-8");
    std::string fileKeyName="./key.txt";
	FILE *fp;
	int bufsize=120;
	if((fp=fopen(fileKeyName.c_str(),"r"))==NULL){
		printf("%s file no exit\n",fileKeyName.c_str());
		exit(0);
	}
	char *oneLine=(char *)malloc(sizeof(char)*bufsize);
	while(fgets(oneLine,bufsize,fp)!=NULL){
		//printf("%s\n",oneLine);
        //oneLine[strlen(oneLine)]='\0';

        string iterm(oneLine, strlen(oneLine));
        iterm = GentUtil::Trim(iterm);
        char abc[120]={0};
        memcpy(abc,iterm.c_str(),iterm.size());
        wchar_t tmp[bufsize];
		size_t wc_len = GentFindUtil::Charwchar(abc,tmp);
        //cout << "len: "<< wc_len <<endl;
		ItemCreate(tmp,wc_len);
        //break;
	}
	fclose(fp);

	string str="";
	GentFind f;
	std::ifstream fin("data.txt", ios::in);
	//char c[4096];
    string st;
	while(!fin.eof()){
		//fin.read(c,4096);
		//string c2(c);
		fin >> st;
        str+=st;
	}
    //cout << str << endl;
    
    f.RemoveChar(str);
	fin.close();
	//vector<string> v;
	//f.Search(str, v);

    exit(1);
}

                                                                     

//分配内存                                               
void  GentFindMgr::IncreMemary(int cur_len)                   
{                                                        
	int last = cur_len*2;                                
	void *p = GentFindUtil::Gmalloc(last*sizeof(node *));            
	p = memcpy(p,nodestable, sizeof(node *)*length);     
	GentFindUtil::Gfree(nodestable);                                 
	nodestable = (node **)p;                                      
	length = last;                                     
}                                                        

//获得孩子的数目                                         
int GentFindMgr::GetChildCount(int parent_key) {
    nodestats_t::iterator it;
    it = nodestats.find(parent_key);
    if(it == nodestats.end()) {
        return 0;
    }
    return nodestats[parent_key].size();
}                                                        

void GentFindMgr::GetChild(int parent_index, std::vector<int> &ret) {
	nodestats_t::iterator it;
    it = nodestats.find(parent_index);
    if(it == nodestats.end()) {
        return;
    }
    ret = nodestats[parent_index];
    /*
    for(size_t k =0; k<nodestats[parent_index].size();k++){
        ret.push_back(nodestats[parent_index][k]);
    }
     */
}                                                        

/**
 *@parent_index old node index
 *@new_index new node index
 */
void GentFindMgr::SetChildCheck(int parent_index,int new_index) {
    //find parent_index node's child
    nodestats_t::iterator it;
    it = nodestats.find(parent_index);
    if(it == nodestats.end()) {
        return;
    }
    for(size_t j=0; j<nodestats[parent_index].size();j++) {
        nodestable[nodestats[parent_index][j]]->check = new_index;
    }
    
    /*
    int i;                                              
    queue_t *q = nodestats.head_ex;                     
    do{                                                 
        i = q->index;                                   
        if(nodestable[i]->check == parent_index) {      
            nodestable[i]->check = val;                 
        }
		q = q->next;                                               
    }while(q);
    */
}                                                       

int GentFindMgr::GetBaseValue(int parent_index,const char *key,
                              int is_asc,std::vector<int> &child) {
    int cur_base = nodestable[parent_index]->base;
    int tmp_base = cur_base+1;                                                                     
    //找出child节点
	GetChild(parent_index,child);
    //std::vector<int>::iterator it;
	size_t j=0;
	int is_find=0;                                                                                 
	while(!is_find) {                                                                              
		int is_able = 1;
        //for(it=child.begin();it!=child.end();it++){
        for(j=0; j<child.size(); j++) {
            int pos_index = child[j] - cur_base + tmp_base;
			//分配内存                                                                             
			if(length < pos_index){                                                                
				IncreMemary(pos_index);
			}else if(nodestable[pos_index] != 0){                                                  
				//表示该节点不可以存放                                                             
				is_able = 0;                                                                       
				tmp_base++;                                                                        
				break;                                                                             
			}                                                                                      
		}                                                                                          
		if(is_able == 0) continue;                                                                 
		//已经找到                                                                                 
		if(key == NULL) break;                                                                     
		//找到了,但同时也需要保证新增加节点也是空的                                                
		int t = GetEncode(key,tmp_base,is_asc);                                                   
		if(!nodestable[t]) break;                                                                  
		tmp_base++;                                                                                
	}                                                                                              
	nodestable[parent_index]->base = tmp_base;                                                     
	return tmp_base;                                                                               
}                                                                                                  

int GentFindMgr::MoveNode(int child_count,std::vector<int> &child,int real_base,int is_asc,
                          int index,int parent_index) {
    int ret = index;
    std::vector<int> new_child;
    size_t childCount = child.size();
	for(size_t i=0; i<childCount; i++){
		int new_index = GetEncode(nodestable[child[i]]->name,real_base,is_asc);                                             
        nodestable[new_index] = NodeSet(nodestable[child[i]]->base,nodestable[child[i]]->check,                             
				nodestable[child[i]]->child_count,nodestable[child[i]]->name,                    
				nodestable[child[i]]->is_word);                                                  
		//可能是移动父节点                                                                                                   
		if(child[i] == index) {                                                                                              
			ret = new_index;                                                                                                 
		}
        new_child.push_back(new_index);
		//改变new_index的孩子节点的check值                                                                                   
		SetChildCheck(child[i],new_index);
        //取得节点的孩子index把赋值给新的节点
        std::vector<int> vt;
        GetChild(child[i], vt);
		AddExQueue(new_index,vt);
        //删除老的节点
		DelExQueue(child[i]);
		node *p = nodestable[child[i]];                                                                                      
		nodestable[child[i]] = NULL;                                                                                         
		GentFindUtil::Gfree(p);                                                                                                          
	}
    AddExQueue(parent_index, new_child);
	return ret;                                                                                                              
}                                                                                                                            

long GentFindMgr::NodesConflict(long encode_t, const char *name,int index,int is_asc) {
    //寻找节点,冲突发生                                                                                                           
    //检查t节点的check值，（查看t有多少个兄弟节点）
    //此时encode_t是新节点和index还未建立真正的父子关系
	int tcount = GetChildCount(nodestable[encode_t]->check);                                                                    
	//查看index节点的孩子数目                                                                                                     
	int icount = GetChildCount(index);                                                                                          
	//包括需要插入的节点                                                                                                          
	icount++;                                                                                                                     
	//printf("check:%d,tcount:%d , icount:%d\n",nodestable[encode_t]->check,tcount,icount);
    
	//改变child小的那个base值,且 新的节点
	if(tcount<=icount && encode_t != index){                                                                                      
		//让出encode_t节点,为了不使用改变index(父节点)，所以不能有encode_t == index （与父节点冲突）                              
        std::vector<int> child;
        
		//寻找数组dArray[dArray[t].check]的base值，所有孩子当中check[］=0,base[]=0，使当前t节点为可用   (dArray[t].check 为父节点)
		int real_base = GetBaseValue(nodestable[encode_t]->check,NULL,is_asc,child);                                     
		//get_child(nodestable[encode_t]->check,tcount);                                                                          
		//把子节点的数据挪到新的位置, dArray[childArr[i]]为老的节点                                                               
		//printf("index2:%d,index_base2:%d\n",index,nodestable[index]->base);                                                     
		//特殊情况：孩子节点当中正好有index节点，哈哈哈 => 需要改变index的值                                                      

		index = MoveNode(tcount,child,real_base,is_asc,index, nodestable[encode_t]->check);
        AddExQueue(index,encode_t);                                                           
		nodestable[encode_t] = NodeSet(nodestable[index]->base,index,0,name,0);                                                  
	}else{                                                                                                                        
		//改变index的base值.来达到index的子节点(包括当前需要插入的节点)，同时把新的插入节点(不是t)
		//t的值是index新的base值加encode的值
		icount=icount-1;
		//int child[icount];
        std::vector<int> child;
		int real_base = GetBaseValue(index/*,icount*/,name,is_asc,child);
		//改变t的值                                                                                                               
		encode_t = GetEncode(name,real_base,is_asc);                                                                             
		//printf("index: %d , base: %d\n",index,realBase);                                                                        
		//get_child(index,icount);                                                                                                
		//把字节点的数据挪到新的位置, dArray[childArr[i]]为老的节点                                                               
		index = MoveNode(icount,child,real_base,is_asc,index, index);
        //new_child.push_back(encode_t);
        //DelExQueue(index);
        AddExQueue(index,encode_t);
		nodestable[encode_t] = NodeSet(nodestable[index]->base,index,0,name,0);                                                  
	}                                                                                                                             
	return encode_t;                                                                                                              
}

int GentFindMgr::NodesAdd(char *name,int index,int is_asc) {
	long encode_t = GetEncode(name,nodestable[index]->base,is_asc);
	if(length <= encode_t ){
		//分配内存
		IncreMemary(encode_t);
		//给节点赋值
		nodestable[encode_t] = NodeSet(1,index,0,name,0);
		AddExQueue(index, encode_t);
		//display();
		return encode_t;
	}
	node *p = nodestable[encode_t];
	if(p == 0) {
		nodestable[encode_t] = NodeSet(1,index,0,name,0);
		AddExQueue(index, encode_t);
		//display();
		return encode_t;
	}
    
	//该节点是重复的节点，不需要处理
	if(p->check == index && p->base != 0) return encode_t;
    
	//conflict occur
	//printf("index:%d,index_base:%d\n",index,nodestable[index]->base);
	encode_t = NodesConflict(encode_t, name, index, is_asc);
    //	display();
	return encode_t;
}

void GentFindMgr::ItemCreate(wchar_t *name,size_t name_len)
{
	//wprintf(L"%s",name);
	size_t len = name_len;
	size_t i;
    std::vector<int> c;
	int parent = 0;
    c.push_back(0);
	for(i=0; i<len; i++) {
		nodestable[parent]->child_count++;
		if(name[i] < 128) {
			char buff[2];
			GentFindUtil::Wcstombs(buff,2,&name[i]);
            //cout<< buff << endl;
			if(strcmp(buff,"\n") == 0 || strcmp(buff," ") == 0) break;
			parent=NodesAdd(buff,parent,1);
		}else{
			char buff[4];
			GentFindUtil::Wcstombs(buff, 4, &name[i]);
           // cout<< buff << endl;
			parent=NodesAdd(buff,parent,0);
		}
         c.push_back(parent);
	}
    /*
    for(size_t j=0;j<c.size(); j++){
        cout << "parent:" << nodestable[c[j]]->name << "\t" << c[j] << endl;
        cout << "child:" << nodestats[c[j]].size() << endl;
        for(size_t k=0;k<nodestats[c[j]].size(); k++) {
            int a = nodestats[c[j]][k];
            cout << "cj: "<<a << endl;
        }
    }
    */

	nodestable[parent]->is_word = 1;                                   

}

int GentFindMgr::ItemSearch(char *name,int base_index,int is_asc) {
	if(nodestable[base_index] == 0) return -1;
	int index = GetEncode(name, nodestable[base_index]->base, is_asc);
	if(length < index) return -1;
	if(nodestable[index] == 0) return -1;
	if(nodestable[index]->check != base_index){
		return -1;
	}
	return index;
}

short GentFindMgr::ItemAttr(int index,const string &field) {
	if(field == "child_count"){
	   return nodestable[index]->child_count;
	}else if(field == "is_word") {
		return nodestable[index]->is_word;
	}
	return -1;
}

GentFind::GentFind() {

}

GentFind::~GentFind(){
                                        
}

void GentFind::stack_init() {
	stack_s = (stack *)GentFindUtil::Gmalloc(sizeof(stack));
	stack_s->head = NULL;
	stack_s->tail = NULL;
	ret_s = (ret *)GentFindUtil::Gmalloc(sizeof(ret));
	ret_s->head = NULL;
	ret_s->tail = NULL;
}

void GentFind::stack_free() {
	item_st *it = stack_s->head;
	item_st *its;
	while(it) {
		its = it->next;
		GentFindUtil::Gfree(it);
		it = its;
	}
	stack_s->head = NULL;
	stack_s->tail = NULL;
}

void GentFind::stack_push(char *name,int len) {
	item_st *it = (item_st *)GentFindUtil::Gmalloc(sizeof(item_st));
	memcpy(it->name,name,len);
	if(stack_s->head == NULL) {
		stack_s->head = it;
		stack_s->tail = it;
	}else {
		stack_s->tail->next = it;
		stack_s->tail = it;
	}
}

void GentFind::stack_pop() {
	item_ret *ret_it = (item_ret *)GentFindUtil::Gmalloc(sizeof(item_ret));
	char *buf = (char *)GentFindUtil::Gmalloc(50);
	item_st *it = stack_s->head;
	strcpy(buf,"");
	while(it) {
		strcat(buf,it->name);
		it = it->next;
	}
	ret_it->key = buf;
	if(ret_s->head == NULL) {
		ret_s->head = ret_it;
		ret_s->tail = ret_it;
	}else {
		ret_s->tail->next = ret_it;
		ret_s->tail = ret_it;
	}
	stack_free();
}

void GentFind::RemoveChar(string &str) {
    string special[4] = {"\t"," ","\r"};
    string rep = "\n";
    for(int i=0; i<4; i++) {
        size_t pos=0;
        while((pos = str.find_first_of(special[i], pos))!=string::npos) {
            cout << "pos:" << pos<< endl;
            str.replace(pos, 1, rep);
            pos += 1;
        }
    }
    cout << str << endl;
    size_t pos = 0;
    size_t pre = 0;
    std::vector<string> str_parts;
    while((pos = str.find_first_of(rep, pos))!=string::npos) {
        string s = str.substr(pre, pos-pre);
        if(s!="" && s != rep) str_parts.push_back(s);
        //cout << "--------------------" << endl;
        //cout << s << endl;
        
        pre = pos;
        pos++;
    }
    string s = str.substr(pre);
    if(s!="" && s != rep) str_parts.push_back(s);
    
    for(size_t j=0;j<str_parts.size(); j++) {
        //cout << "parts:" << j << " len:"<<str_parts[j].size()<<"   "<< str_parts[j]  << endl;
        std::vector<string> ret;
        Search(str_parts[j], ret);
        
    }
    
}

int GentFind::Search(string &str, std::vector<string> &v) {
	//cout << "GentFind::Search "<< str << endl;
   	wchar_t *buff = (wchar_t *)GentFindUtil::Gmalloc(sizeof(wchar_t)*str.size()+1);
	char *str2 = const_cast<char *>(str.c_str());
	size_t wc_len = GentFindUtil::Charwchar(str2,buff);
	if(wc_len == -1) {
		GentFindUtil::Gfree(buff);
		return -1;
	}
	stack_init(); 
	size_t i = 0;
	int index = 0;
	int tindex;
	int pos = 0;
	int rsize = 0;
	while(i < wc_len){
		char c[4];
		int len;
		if(*(buff+i) < 128) {
			GentFindUtil::Wcstombs(c,2,buff+i);
			if(strcmp(c," ") == 0){
				printf("kong\n");
				if(stack_s->head != NULL && GentFindMgr::Instance()->ItemAttr(index,"is_word") == 1){
					stack_pop();
					pos = 0;
				}
				i++;
				continue;
			}
			tindex = GentFindMgr::Instance()->ItemSearch(c,index,1);
			len = 2;
		}else {
			GentFindUtil::Wcstombs(c, 4, buff+i);
			tindex = GentFindMgr::Instance()->ItemSearch(c,index,0);
			len = 4;
		}	
		if(tindex == -1){
			//没有找到，检查上一个节点是否为最后一个节点
			if(stack_s->head != NULL && GentFindMgr::Instance()->ItemAttr(index,"is_word") == 0) {
				//表示上个节点不是个词组，回溯到节点的下一个字位置
				i = pos + 1;
				pos = 0;
				stack_free();
			}else if(stack_s->head != NULL && GentFindMgr::Instance()->ItemAttr(index,"is_word") == 1) {
				//上一次是一个词组，把词组保存
				stack_pop();
				//i的值不用加

				pos = 0;
			} else {
				i++;
			}
			//从开始寻找
			index = 0;
		}else {
			if(index == 0) {
				//记住当前位置
				pos = i;
			}
			//压栈
			stack_push(c,len);
			i++;
			index = tindex;
		}
	}
    
    if(tindex != -1) {
        if(stack_s->head != NULL && GentFindMgr::Instance()->ItemAttr(index,"is_word") == 1){
            stack_pop();
        }
    }
    item_ret *ret_it = ret_s->head;
    while(ret_it) {
        printf("find: %s\n",ret_it->key);
        rsize += strlen(ret_it->key);
        //strcat(ret, ret_it->key);
        //strcat(ret, "\n");
        rsize++;
        ret_it = ret_it->next;
    }
    free(buff);
    return rsize;

}
