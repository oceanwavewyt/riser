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

