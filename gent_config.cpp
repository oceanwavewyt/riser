/*
 * gent_config.cpp
 *
 *  Created on: 2012-7-7
 *      Author: wyt
 */
#include "gent_config.h"
#include "prefine.h"
GentConfig *GentConfig::instance_ = NULL;

GentConfig *GentConfig::Instance() {
	if(NULL == instance_) {
		instance_ = new GentConfig();
	}
	return instance_;
}

void GentConfig::Unstance() {
	if(instance_ != NULL) {
		delete instance_;
	}
}

GentConfig::GentConfig() {
}


GentConfig::~GentConfig() {
	GentConfig::Unstance();
}

void GentConfig::set(string &key,string &val) {
	conf_[key] = val;
}

string &GentConfig::operator[](const string &key) {
	return conf_[key];
}
