/**
* @author wyt
* 
*/
#include "prefine.h"
#define xisspace(c) isspace((unsigned char)c)

class  GentUtil
{
public:
	static bool SafeStrtol(const char *str, int32_t *out);
    static void AssignVal(const char *, string &outstr, int);
    static string LTrim(const string& str);
    static string RTrim(const string& str);
    static string Trim(const string& str);
};
