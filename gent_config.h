/*
 * gent_config.cpp
 *
 *  Created on: 2012-7-7
 *      Author: wyt
 */

#ifndef GENT_CONFIG_CPP_
#define GENT_CONFIG_CPP_
#include "prefine.h"

class GentConfig {
	static GentConfig *instance_;
public:
	static GentConfig *Instance();
	static void Unstance();
private:
	std::map<string,string> conf_;
public:
	GentConfig();
	~GentConfig();
	void set(string &,string &);
	string &operator [](const string &key);
    void Parse(const string &filename);
};


#endif /* GENT_CONFIG_CPP_ */
