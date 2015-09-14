#ifndef riser_gent_file_h
#define riser_gent_file_h
#include "prefine.h"
#include <sys/mman.h> 
#include <sys/stat.h> 

template <class T>  
class GentFile  
{  
	uint32_t len_;
	CommLock w_lock;
	string filename_;
	T *head_;
	int fd;
public:
	GentFile(const string &filepath, int length)
	{
		filename_ = filepath;
		len_ = length;
	};
	~GentFile()
	{
		munmap(head_, sizeof(T)*len_);
   		close(fd);
	}	
public:
	bool Init(std::map<string, T*> &lead_rep) 
	{
		fd = OpenFile(filename_);
		struct stat st;
		if(stat(filename_.c_str(), &st) == -1) {
			LOG(GentLog::ERROR, "stat %s failed.",filename_.c_str());
			close(fd);
			return false;                              
		}
		if(!st.st_size) {
			if(ftruncate(fd,sizeof(T)*len_) < 0) {
				//LOG(GentLog::ERROR, "ftruncate head.dat failed.");                                          close(fd);
				return false;
			}                                                     
		}
		void *h = mmap(NULL, sizeof(T)*len_, PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0); 
		if (h == MAP_FAILED) {
			//LOG(GentLog::ERROR, "mmap page failed.");
			close(fd);
			return false;
		}  
		head_ = reinterpret_cast<T*>(h);
		
		for(int i=0;i<2; i++) {
			T *h = head_ + sizeof(T)*i;
			if(!h->available) break;
			string s(h->name,h->name_len);
			lead_rep[s] = h; 
		}
		
		return true;  	
	};
	T *AddItem()
	{
		for(uint32_t i=0;i<len_; i++) {
			T *h = head_ + sizeof(T)*i;
			if(h->available) continue;
			return h;
		}
		return NULL;	
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
