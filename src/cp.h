#ifndef _CP_H
#define _CP_H

#include <stdbool.h>

struct flags {
    bool verbose;
    bool no_clobber;
    bool update;
    bool interactive;
    bool recursion;
    bool hard_link;
};

void copy_aux(int argc, char *argv[], struct flags flags);
#endif
