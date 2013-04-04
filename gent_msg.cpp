/*
 * gent_msg.cpp
 *
 *  Created on: 2012-3-25
 *      Author: wyt
 */
#include "prefine.h"
#include "gent_msg.h"
/*
COMM_PACK::COMM_PACK()
{

}

COMM_PACK::~COMM_PACK()
{

}

int COMM_PACK::GetData(std::string &key, std::string &val)
{
	if(data_.count(key)==0) return -1;
	val = data_[key];
	return 0;
}

void COMM_PACK::UrlParse(const char *urls){
	string url(urls);
	unsigned start = url.find("?");
	if(start == string::npos) return;
	ParamParse(url,start+1);
}

void COMM_PACK::ParamParse(std::string &url, unsigned start) {
	unsigned end = url.find("&",start);
	if(end == string::npos) return;
	unsigned pos = 0;
	while(end<=url.size()) {
		string tmp = url.substr(start,end-start);
		pos = tmp.find("=");
		if(pos==string::npos) {
			end = start;
			break;
		}
		string key = tmp.substr(0,pos);
		string val = tmp.substr(pos+1);
		data_[key] = val;
		start = end+1;
		end = url.find("&",start+1);
		if(end == string::npos && start<url.size()) {
			end = url.size();
		}
		cout << "key: " << key << "\t val: " << val << endl;
	}
}

void COMM_PACK::ContentParse(const char *content) {
	string url(content);
	ParamParse(url,0);
}
*/