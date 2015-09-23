/*
 * gentle.h
 *
 *  Created on: 2012-2-27
 *      Author: wyt
 */

#ifndef GENTLE_H_
#define GENTLE_H_
#include "gent_frame.h"

class GentBasic
{
public:
	string resp_str_;
public:
	GentBasic();
	virtual ~GentBasic();
public:
	virtual int Proccess() = 0;
	virtual GentBasic *CloneProccess() = 0;
};


#endif /* GENTLE_H_ */
