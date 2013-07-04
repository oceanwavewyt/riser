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

void *GentFindMgr::Gmalloc(size_t size) {    
	void *p;
	p = malloc(size);             
	if(!p) {
		printf("malloc failed\n");
		return 0;                 
	}
	memset(p,0,size);
	return p;
}   

void *GentFindMgr::Gcalloc(size_t size,int len) {
    void *p;
    p = calloc(size,len);
    if(!p) {
        printf("calloc failed\n");
        exit(0);
    }
    return p;
}

void GentFindMgr::Gfree(void *p) {
    if(p) {
        free(p);
    }
}

int GentFindMgr::Charwchar(char *str,wchar_t *out) {
	int ret = mbstowcs(out,str,strlen(str));
	if(ret == -1) {
		printf("charwchar failed \n");  
	}
	return ret;
}   

size_t GentFindMgr::Len(wchar_t *str) {
	return wcslen(str);
}   

size_t GentFindMgr::Wcstombs(char *buf,int buf_size,wchar_t *str) {
	size_t ntotal = wcstombs(buf,str,buf_size);
	if(ntotal == 0) {
		printf("wcstombs failed \n");                           
	}
	*(buf+buf_size-1)='\0';
	return ntotal;                                              
}       


node *GentFindMgr::NodeSet(int base,int check,int account,const char *name,short is_word) { 
	node *it = (node *)Gcalloc(1,sizeof(node));                                      
	it->base = base;                                                                   
	it->check = check;                                                                 
	it->child_count = account;                                                         
	it->is_word = is_word;                                                             
	strcpy(it->name,name);                                                             
	return it;                                                                         
}                                                                                      

void GentFindMgr::AddExQueue(int index) {
	int is_exist = 0;
	queue_t *q = nodestats.head_ex;
	do{
		if(!q) break;
		if(q->index == index) {
			is_exist = 1;
			break;
		}
		q = q->next;
	}while (q);
	if(is_exist == 0) {
		q = (queue_t *)Gcalloc(1,sizeof(queue_t));
		q->next = nodestats.head_ex;
		q->index = index;
		nodestats.head_ex = q;
	}
	//q = nodestats.head_ex;
	//do{
	//  printf("%d\n",q->index);
	//}while (q = q->next);
}

void GentFindMgr::DelExQueue(int index) {                                                
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
	nodestable = (node**)Gmalloc(length*sizeof(node *));
	memset(nodestable,0,length*sizeof(node *));   
	nodestable[0] = NodeSet(1,1,0,"",0);       
	AddExQueue(0);                            
	nodestats.head_ex = NULL;                   
}

int GentFindMgr::NodesAdd(char *name,int index,int is_asc) {                
	long encode_t = GetEncode(name,nodestable[index]->base,is_asc);   
	if(length <= encode_t ){                                           
		//分配内存                                                     
		IncreMemary(encode_t);                                        
		//给节点赋值                                                   
		nodestable[encode_t] = NodeSet(1,index,0,name,0);             
		AddExQueue(encode_t);                                        
		//display();                                                     
		return encode_t;                                               
	}                                                                  
	node *p = nodestable[encode_t];                                    
	if(p == 0) {                                                       
		nodestable[encode_t] = NodeSet(1,index,0,name,0);             
		AddExQueue(encode_t);                                        
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

//分配内存                                               
void  GentFindMgr::IncreMemary(int cur_len)                   
{                                                        
	int last = cur_len*2;                                
	void *p = Gmalloc(last*sizeof(node *));            
	p = memcpy(p,nodestable, sizeof(node *)*length);     
	Gfree(nodestable);                                 
	nodestable = (node **)p;                                      
	length = last;                                       
}                                                        

//获得孩子的数目                                         
int GentFindMgr::GetChildCount(int parent_key) {             
	int ret = 0;                                         
	int i;                                               
	queue_t *q = nodestats.head_ex;                      
	do{                                                  
		i = q->index;                                    
		if(nodestable[i]->check == parent_key) {         
			ret++;                                       
		}
		q = q->next;                                                
	}while(q);                                 
	return ret;                                          
}                                                        

void GentFindMgr::GetChild(int parent_index,int ret[]) {      
	int i;                                               
	int j = 0;                                           
	queue_t *q = nodestats.head_ex;                      
	do{                                                  
		i = q->index;                                    
		if(nodestable[i]->check == parent_index) {       
			ret[j++] = i;                                
		}                                                
		q = q->next;
	}while(q);                                 
}                                                        

void GentFindMgr::SetChildCheck(int parent_index,int val) { 
    int i;                                              
    queue_t *q = nodestats.head_ex;                     
    do{                                                 
        i = q->index;                                   
        if(nodestable[i]->check == parent_index) {      
            nodestable[i]->check = val;                 
        }
		q = q->next;                                               
    }while(q);                                
}                                                       

int GentFindMgr::GetBaseValue(int parent_index,int child_count,const char *key,int is_asc,int child[]) {                                                   int cur_base = nodestable[parent_index]->base;                                                 
    int tmp_base = cur_base+1;                                                                     
    //找出child节点                                                                                
	GetChild(parent_index,child);                                                                 
	int j=0;                                                                                       
	int is_find=0;                                                                                 
	while(!is_find) {                                                                              
		int is_able = 1;                                                                           
		for(j=0; j<child_count; j++) {                                                             
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

int GentFindMgr::MoveNode(int child_count,int child[],int real_base,int is_asc,int index) {                                       
    int i;                                                                                                                   
    int ret = index;                                                                                                         
	for(i=0; i<child_count; i++){                                                                                            
		int pindex = nodestable[child[i]]->check;                                                                            
		int tmp_index = child[i] - nodestable[pindex]->base + real_base;                                                     
		int new_index = GetEncode(nodestable[child[i]]->name,real_base,is_asc);                                             
		printf("tmp_index:%d,new_index:%d,base:%d\n",tmp_index,new_index,nodestable[child[i]]->base);                      
		nodestable[new_index] = NodeSet(nodestable[child[i]]->base,nodestable[child[i]]->check,                             
				nodestable[child[i]]->child_count,nodestable[child[i]]->name,                    
				nodestable[child[i]]->is_word);                                                  
		//可能是移动父节点                                                                                                   
		if(child[i] == index) {                                                                                              
			ret = new_index;                                                                                                 
		}                                                                                                                    
		//改变new_index的孩子节点的check值                                                                                   
		SetChildCheck(child[i],new_index);                                                                                 
		AddExQueue(new_index);                                                                                             
		DelExQueue(child[i]);                                                                                              
		node *p = nodestable[child[i]];                                                                                      
		nodestable[child[i]] = NULL;                                                                                         
		Gfree(p);                                                                                                          
	}                                                                                                                        
	return ret;                                                                                                              
}                                                                                                                            

long GentFindMgr::NodesConflict(long encode_t, const char *name,int index,int is_asc) {                                                
    //寻找节点,冲突发生                                                                                                           
    //检查t节点的check值，（查看t有多少个兄弟节点）                                                                               
	int tcount = GetChildCount(nodestable[encode_t]->check);                                                                    
	//查看index节点的孩子数目                                                                                                     
	int icount = GetChildCount(index);                                                                                          
	//包括需要插入的节点                                                                                                          
	icount++;                                                                                                                     
	//printf("check:%d,tcount:%d , icount:%d\n",nodestable[encode_t]->check,tcount,icount);                                       
	//改变child小的那个base值,且 新的节点                                                                                         
	if(tcount<=icount && encode_t != index){                                                                                      
		//让出encode_t节点,为了不使用改变index(父节点)，所以不能有encode_t == index （与父节点冲突）                              
		int child[tcount];                                                                                                        
		//寻找数组dArray[dArray[t].check]的base值，所有孩子当中check[］=0,base[]=0，使当前t节点为可用   (dArray[t].check 为父节点)
		int real_base = GetBaseValue(nodestable[encode_t]->check,tcount,NULL,is_asc,child);                                     
		//get_child(nodestable[encode_t]->check,tcount);                                                                          
		//把子节点的数据挪到新的位置, dArray[childArr[i]]为老的节点                                                               
		//printf("index2:%d,index_base2:%d\n",index,nodestable[index]->base);                                                     
		//特殊情况：孩子节点当中正好有index节点，哈哈哈 => 需要改变index的值                                                      

		index = MoveNode(tcount,child,real_base,is_asc,index);                                                                   
		//int b = (nodestable[index] == 0)?1:nodestable[index]->base;                                                             
		nodestable[encode_t] = NodeSet(nodestable[index]->base,index,0,name,0);                                                  
	}else{                                                                                                                        
		//改变index的base值.来达到index的子节点(包括当前需要插入的节点)，同时把新的插入节点(不是t)                                
		//t的值是index新的base值加encode的值                                                                                      
		icount=icount-1;                                                                                                          
		int child[icount];                                                                                                        
		int real_base = GetBaseValue(index,icount,name,is_asc,child);                                                           
		//改变t的值                                                                                                               
		encode_t = GetEncode(name,real_base,is_asc);                                                                             
		//printf("index: %d , base: %d\n",index,realBase);                                                                        
		//get_child(index,icount);                                                                                                
		//把字节点的数据挪到新的位置, dArray[childArr[i]]为老的节点                                                               
		index = MoveNode(icount,child,real_base,is_asc,index);                                                                   
		nodestable[encode_t] = NodeSet(nodestable[index]->base,index,0,name,0);                                                  
	}                                                                                                                             
	AddExQueue(encode_t);                                                                                                       
	return encode_t;                                                                                                              
}

void GentFindMgr::ItemCreate(wchar_t *name,size_t name_len)
{
	wprintf(L"%s\n",name);
	size_t len = name_len;
	size_t i;
	int parent = 0;
	for(i=0; i<len; i++) {
		nodestable[parent]->child_count++;
		if(name[i] < 128) {
			char buff[2];
			Wcstombs(buff,2,&name[i]);
			if(strcmp(buff,"\n") == 0 || strcmp(buff," ") == 0) break;
			parent=NodesAdd(buff,parent,1);
		}else{
			char buff[4];
			Wcstombs(buff, 4, &name[i]);
			parent=NodesAdd(buff,parent,0);
		}
	}

	nodestable[parent]->is_word = 1;                                   

}
GentFind::GentFind() {

}

GentFind::~GentFind(){
                                        
}

void GentFind::Search(const string &str, std::vector<string> &v) {
	cout << "GentFind::Search "<< str << endl;

}
