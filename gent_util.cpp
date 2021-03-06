#include "prefine.h"
#include "gent_util.h"

bool GentUtil::SafeStrtol(const char *str, int32_t *out) {
    assert(out != NULL);
    errno = 0;
    *out = 0;
    char *endptr;
    long l = strtol(str, &endptr, 10);
    if ((errno == ERANGE) || (str == endptr)) {
        return false;
    }
    
    if (xisspace(*endptr) || (*endptr == '\0' && endptr != str)) {
        *out = l;
        return true;
    }
    return false;
}

void GentUtil::AssignVal(const char *str, string &outstr, int len) {
    outstr.assign(str,0,len);
}

string GentUtil::LTrim(const string& str) {
	uint64_t s = str.find_first_not_of(" \n\r\t");
	return str.substr(s);
}

string GentUtil::RTrim(const string& str) {
    return str.substr(0,str.find_last_not_of(" \n\r\t")+1);
}

string GentUtil::Trim(const string& str) {
    return LTrim(RTrim(str));
}

bool GentUtil::Split(const string &str, const string &delimit, vector<string> &v) {
	uint64_t rpos, pos = 0;
	string s;
	while((rpos = str.find_first_of(delimit, pos)) != string::npos) {
		if(rpos == pos) {
			pos++;
		}else{
			s = Trim(str.substr(pos, rpos-pos));
			if(s != "") { 
				v.push_back(s);
			}
			pos = rpos+1;
		}
	}
	s = Trim(str.substr(pos));
	if(s != "") { 
		v.push_back(s);
	}		
	return true;
}

void GentUtil::BytesToHuman(char *s, unsigned long long n) {
    double d;

    if (n < 1024) {
        /* Bytes */
        sprintf(s,"%lluB",n);
        return;
    } else if (n < (1024*1024)) {
        d = (double)n/(1024);
        sprintf(s,"%.2fK",d);
    } else if (n < (1024LL*1024*1024)) {
        d = (double)n/(1024*1024);
        sprintf(s,"%.2fM",d);
    } else if (n < (1024LL*1024*1024*1024)) {
        d = (double)n/(1024LL*1024*1024);
        sprintf(s,"%.2fG",d);
    } else if (n < (1024LL*1024*1024*1024*1024)) {
        d = (double)n/(1024LL*1024*1024*1024);
        sprintf(s,"%.2fT",d);
    } else if (n < (1024LL*1024*1024*1024*1024*1024)) {
        d = (double)n/(1024LL*1024*1024*1024*1024);
        sprintf(s,"%.2fP",d);
    } else {
        /* Let's hope we never need this */
        sprintf(s,"%lluB",n);
    }
}

string GentUtil::TimeToStr(size_t tm)
{
	time_t times = (time_t)tm;
	struct tm *lt = localtime(&times);  
    char nowtime[24]={0};  
    memset(nowtime, 0, sizeof(nowtime));  
    strftime(nowtime, 24, "%Y-%m-%d %H:%M:%S", lt); 		
	string str(nowtime);
	return str;
}

