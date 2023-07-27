
#include "as_string.h"

char *as_strsep(char **stringp, const char *delim)
{
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    return strsep(stringp,delim);
#elif AS_APP_OS == AS_OS_WIN32
    char *s;
    const char *spanp;
    int c, sc;
    char *tok;
    if ((s = *stringp)== NULL)
        return (NULL);
    for (tok = s;;) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc =*spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
#endif
}
