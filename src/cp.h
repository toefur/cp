#ifndef _CP_H
#define _CP_H

#include <stdbool.h>

#define CP_OPTS "filnRrsuv"

struct flags {
    bool force;
    bool interactive;
    bool link;
    bool no_clobber;
    bool recursion;
    bool symlink;
    bool update;
    bool verbose;
};

void copy_aux(int argc, char *argv[], struct flags flags);
#endif
