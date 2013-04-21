/*
 * gent_config.cpp
 *
 *  Created on: 2012-7-7
 *      Author: wyt
 */
#include <fstream>
#include "gent_config.h"
#include "prefine.h"


GentConfig::GentConfig() {
	noexist = "";
    conf_["port"] = "3535";
    conf_["thread"] = "10";
}


GentConfig::~GentConfig() {

}

void GentConfig::set(string &key,string &val) {
	conf_[key] = val;
}

string &GentConfig::operator[](const string &key) {
	std::map<string,string>::iterator iter;
	iter = conf_.find(key);
	if(iter == conf_.end()) {
		return noexist;
	}
	/*
    for (iter = conf_.begin(); iter != conf_.end(); iter++)
        std::cout << iter->first << " = " << iter->second << std::endl;
	*/
	return conf_[key];
}

void GentConfig::Parse(const string &filename) {
    std::string s, key, value;
    ifstream ins(filename.c_str());
    // For each (key, value) pair in the file
    while (std::getline( ins, s ))
    {
        std::string::size_type begin = s.find_first_not_of(" \f\t\v");        
        // Skip blank lines
        if (begin == std::string::npos) continue;
        // Skip commentary
        if (std::string( "#;" ).find( s[ begin ] ) != std::string::npos) continue;
        
        // Extract the key value
        std::string::size_type end = s.find( '=', begin );
        key = s.substr( begin, end - begin );
        
        // (No leading or trailing whitespace allowed)
        key.erase( key.find_last_not_of( " \f\t\v" ) + 1 );
        
        // No blank keys allowed
        if (key.empty()) continue;
        
        // Extract the value (no leading or trailing whitespace allowed)
        begin = s.find_first_not_of( " \f\n\r\t\v", end + 1 );
        end   = s.find_last_not_of(  " \f\n\r\t\v" ) + 1;
        
        value = s.substr( begin, end - begin );
        
        // Insert the properly extracted (key, value) pair into the map
        conf_[ key ] = value;
    }
    ins.close();

}
