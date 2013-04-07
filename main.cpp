//
//  main.cpp
//  riser
//
//  Created by wyt on 13-1-20.
//  Copyright (c) 2013年 wyt. All rights reserved.
//

#include "gent_util.h"
#include "gent_frame.h"
#include "gent_config.h"

void Split(const string &str, const string &delimit) {
    vector<string> v;
    int pos,last_pos=0;
    while((pos = str.find_first_of(delimit,last_pos)) != string::npos){
        if(pos == last_pos){
            last_pos++;
            cout << "last_pos1: " << last_pos << endl;
        }else{
            v.push_back(str.substr(last_pos, pos-last_pos));
            last_pos = pos+1;
            cout << "last_pos2: " << last_pos << endl;
        }
    }
    cout << "str.size(): " << str.size() << "last_pos: " << last_pos << endl;
    if(str.size()!=last_pos){
        string curstr = str.substr(last_pos);
        cout<< "curstr: "<< endl;
        v.push_back(curstr);
    }
    vector<string>::iterator iter;
    for(iter=v.begin(); iter!=v.end();iter++){
       cout<<"split: " <<  *iter << endl;
    }
}

int main(int argc, const char * argv[])
{
    //char buf[100];
    //sprintf(buf,"get  abc  ");
    //Split(string(buf,strlen(buf))," ");
    //return 1;
    // insert code here...
    std::cout << "Hello, World!\n";
    //GentFrame::Instance()->Run(atoi(argv[1]));
    if(!GentFrame::Instance()->Init()){
        std::cout << "init fail!\n";
        return 1;
    }
    std::cout << GentFrame::Instance()->config["abc"] << endl;
    GentFrame::Instance()->config["abc"] = "test";
     std::cout << GentFrame::Instance()->config["abc"] << endl;
    GentFrame::Instance()->Run(10);
    return 0;
}

