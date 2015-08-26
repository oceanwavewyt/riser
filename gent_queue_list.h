#ifndef riser_gent_queue_list_h
#define riser_gent_queue_list_h
#include "prefine.h"  
  
//定义队列的节点结构  
template <class T>  
struct NODE  
{  
    NODE<T>* next;  
    T data;  
};  
  
template <class T>  
class GentListQueue  
{  
	uint64_t length;
	CommLock len_lock;
public:  
    GentListQueue()  
    {  
        NODE<T>* p = new NODE<T>;  
        if (NULL == p)  
        {  
            cout << "Failed to malloc the node." << endl;  
        }  
        p->data = NULL;  
        p->next = NULL;  
        front = p;  
        rear = p;
		length = 0;  
    }  
  
//在队尾入队  
    void push(T e)  
    {  
        NODE<T>* p = new NODE<T>;  
        if (NULL == p)  
        {  
            cout << "Failed to malloc the node." << endl;  
        }  
        p->data = e;  
        p->next = NULL;  
        rear->next = p;  
        rear = p;
		AutoLock lock(&len_lock); 
		length++; 
    }  
  
//在队头出队  
    T pop()  
    {  
        T e;  
  
        if (front == rear)  
        {  
            cout << "The queue is empty." << endl;  
            return NULL;  
        }  
        else  
        {  
            NODE<T>* p = front->next;  
            front->next = p->next;  
            e = p->data;  
            //注意判断当只有一个元素，且删除它之后，rear指向的node被删除  
            //应将其指向头结点  
            if (rear == p)  
            {  
                rear = front;  
            }  
            delete p; p = NULL;
			AutoLock lock(&len_lock); 
			length--; 
            return e;  
        }  
    }  
  
    //取得队头元素  
    T front_element()  
    {  
        if (front == rear)  
        {  
            cout << "The queue is empty." << endl;  
            return NULL;  
        }  
        else  
        {  
            NODE<T> *p = front->next;  
			return p->data;  
        }  
    }  
  
    T back_element()  
    {  
        if (front == rear)  
        {  
            cout << "The queue is empty." << endl;  
            return NULL;  
        }  
        else  
        {  
            return rear->data;  
        }  
    }  
     
	NODE<T>* search(int runid)
	{
		if(front == rear) return NULL;
		NODE<T>* p = front;
		int isfind = 0;
		while(p != rear) {
			p = p->next;
			if(p->data->time < runid) continue;
			isfind = 1;
			break;
		}			
		if(isfind == 1) return p;
		return NULL;
	} 
    //取得队列元素个数  
    int size()  
    {  
        int count(0);  
  
        NODE<T>* p = front;  
  
        while (p != rear)  
        {  
            p = p->next;  
            count++;  
        }  
        return count;  
    }  
      
    //判断队列是否为空  
    bool empty()  
    {  
        if (front == rear)  
        {  
            return true;  
        }  
        else  
        {  
            return false;  
        }  
    }  
  
private:  
    NODE<T>* front; //指向头结点的指针。 front->next->data是队头第一个元素。  
    NODE<T>* rear;//指向队尾（最后添加的一个元素）的指针  
};  
 
class itemData {
public:
	enum optType {ADD=0,DEL=1};
	string name;
	int type;
	int tm;
public:
	itemData(const string &key, int optype)
	{
		name = key;
		type = optype;
		tm = time(NULL);
	}	
	~itemData(){};
};
/* 
int main(int argc,char* argv[])  
{  
    ListQueue<itemData*> myqueue;  
    cout << myqueue.size() << endl;  
	itemData it("first",itemData::DEL);
    myqueue.push(&it);
	
	itemData it2("second",2); 
    myqueue.push(&it2);

	itemData it3("three",3); 
    myqueue.push(&it3);  
    cout << myqueue.front_element()->name << endl;  
    cout << myqueue.back_element()->name << endl;  
    myqueue.pop();  
    if (myqueue.empty())  
    {  
        cout << "The queue is empty now." << endl;  
    }  
    else  
    {  
        cout << "The queue has " << myqueue.size() << " elements now." << endl;  
    }  
    myqueue.pop();  
    myqueue.pop();  
    if (myqueue.empty())  
    {  
        cout << "The queue is empty now." << endl;  
    }  
    else  
    {  
        cout << "The queue has " << myqueue.size() << " elements now." << endl;  
    }  
    return 0;  
} 
*/
#endif 
