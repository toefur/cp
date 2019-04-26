/* cp
 *
 * Authors:
 *     Paige Ignatovich <paige.ignatovich@wsu.edu>
 *     Chris Reed       <christopher.j.reed@wsu.edu>
 *
 * Description:
 *     A simple implementation of the linux 'cp' command.
 *     This implementation offers many of the most common features in the 'cp' program.
 */
#ifndef _CP_H
#define _CP_H

#include <stdbool.h>

#define MAX_BUF_SIZE 1024

#define CP_OPTS   "fhilnrRsuv"
#define USAGE_MSG "usage: %s [-%s] SOURCE... DESTINATION                                           \n"
#define HELP_MSG  USAGE_MSG\
                  "                                                                                \n"\
                  "  -f    force remove file in destination if it cannot be overwritten normally   \n"\
                  "  -h    displays this help message then exits                                   \n"\
                  "  -i    prompts the user before overwritting a file that exists                 \n"\
                  "  -l    create a hard link (like the 'ln' command)                              \n"\
                  "  -n    prevents overwriting files in the destination if they already exist     \n"\
                  "  -r,R  allows following directories to copy files or other directories within  \n"\
                  "  -s    create a symbolic link (like the 'ln -s' command)                       \n"\
                  "  -u    only overwrite destination files if the source is newer                 \n"\
                  "  -v    tells the user what is happening                                        \n"

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
