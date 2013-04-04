/*
 * gent_msg.h
 *
 *  Created on: 2012-3-25
 *      Author: wyt
 */

#ifndef GENT_MSG_H_
#define GENT_MSG_H_

#include "prefine.h"
#include <assert.h>
#include <event.h>
//#include <evhttp.h>
#define SMALL_LEN 1000
#define MAX_LEN 10240

class GentConnect;
//template <class T,class Alloc = std::allocator<T> >
template <class T>
class GentMsg
{
//	typedef typename Alloc::size_type size_type;
	//std::vector<T,Alloc> app_cq_;
	std::vector<T> app_cq_;
	unsigned size_;
	unsigned start_;
	pthread_cond_t empt_cond_;
	pthread_cond_t full_cond_;
	pthread_mutex_t lock_;
public:
	GentMsg():size_(0),start_(0) {
		pthread_mutex_init(&lock_, NULL);
		pthread_cond_init(&empt_cond_, NULL);
		pthread_cond_init(&full_cond_, NULL);
	}
	~GentMsg() {

	}
public:
	void RegQueue(std::string &str) {

	}
	unsigned Getsize() {
		return app_cq_.size();
	}
	unsigned Cursize(){
		return size_;
	}
	void Resize(int size) {
		app_cq_.resize(size);
	}
	void Push(T &appName) {
		//cout <<"push start" << size_ <<endl;
		pthread_mutex_lock(&lock_);
		while(size_ >= app_cq_.size()) {
			cout <<"push wait" <<endl;
			pthread_cond_wait(&empt_cond_, &lock_);
		}
		int index = (start_+size_)%app_cq_.size();
		app_cq_[index] = appName;
		//cout << "index:"<< index <<" push size:" << size_ << endl;
		size_++;
		pthread_cond_signal(&full_cond_);
		//pthread_cond_broadcast(&full_cond_);
		pthread_mutex_unlock(&lock_);
	}
	T Pop() {
		//cout <<"pop start" << size_ <<endl;
		pthread_mutex_lock(&lock_);
		//cout << "pop pre size:" << size_ << endl;
		while(size_<=0){
			//cout << "pop wait " << size_ << endl;
			pthread_cond_wait(&full_cond_, &lock_);
		}
		T appName = app_cq_[start_];
		start_ = (start_+1)%app_cq_.size();
		size_--;
		//cout << "pop size:" << size_ << endl;
		pthread_cond_signal(&empt_cond_);
		//pthread_cond_broadcast(&empt_cond_);
		pthread_mutex_unlock(&lock_);
		return appName;
	}
};

typedef GentMsg<GentConnect *>  GENT_MSG_CONNECT;
//typedef GentMsg<COMM_REP>  GENT_REP_COMM;
#endif /* GENT_MSG_H_ */
