#ifndef riser_gent_file_h
#define riser_gent_file_h
#include "prefine.h"
#include <sys/mman.h> 
#include <sys/stat.h> 

template <class T>
class GentFile  
{  
	uint32_t size_;
	uint32_t page_num_;
	size_t page_size_;
	int len_;
	int page_item_num_;
	string filename_;
	char *head_;
	char *limit_;
	vector<T*> head_list_;
	std::map<string, T*> rep_map_list_;
	T item_;
	int fd;
public:
	GentFile(const string &filepath, int length)
	{
		filename_ = filepath;
		page_size_ = getpagesize();
		float ft = item_.size()*length/page_size_;
		page_num_ = (uint32_t)ft + 1;
		size_ = page_num_ * page_size_;
		len_ = length;
		page_item_num_ = (int)page_size_/item_.size();
	};
	~GentFile()
	{
		munmap(head_, size_);
   		close(fd);
	}	
public:
	bool Init(std::map<string, T*> &lead_rep) 
	{
		fd = OpenFile(filename_);
		struct stat st;
		if(stat(filename_.c_str(), &st) == -1) {
			close(fd);
			return false;                              
		}
		if(!st.st_size) {
			if(ftruncate(fd,size_) < 0) {
		        close(fd);
				return false;
			}                                                     
		}
		void *h = mmap(NULL, size_, PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0); 
		if (h == MAP_FAILED) {
			close(fd);
			return false;
		}
		head_ = reinterpret_cast<char *>(h);
		limit_ = head_ + size_;
		for(int i=0; i<len_; i++) {
			char *t = head_+ item_.size()*i;
			T *c = new T(t);
			head_list_.push_back(c);
			if(c->available) {
				lead_rep[c->name] = c;
				rep_map_list_[c->name] = c;
			}
		}			
		return true;  	
	};
	T *AddItem(const string &str)
	{
		typename std::map<string,T*>::iterator it;
		it = rep_map_list_.find(str);
		if(it != rep_map_list_.end()) {
			return rep_map_list_[str];
		}
		for(int i=0;i<len_; i++) {
			if(head_list_[i]->available) continue;
			head_list_[i]->set(str);
			rep_map_list_[str] = head_list_[i];
			return head_list_[i];
		}	
		return NULL;	
	};
	bool DelItem(const string &name)
	{
		for(int i=0;i<len_; i++) {
			if(head_list_[i]->name != name) continue;
			head_list_[i]->set("available",0);
			typename map<string, T*>::iterator it;
			it = rep_map_list_.find(name);
			if(it != rep_map_list_.end()) {
				rep_map_list_.erase(it);	
			}
			return true;
		}			
		return false;
	};
private:
	int OpenFile(string &filename, bool create=true)
	{
		int f = O_RDWR;
		if(access(filename.c_str(),0) == -1) {
			assert(create);
			f = f|O_CREAT;
		}
		int fid = open(filename.c_str(), f, 00644);
		assert(fid>0);
		return fid;
	};

};

#endif
