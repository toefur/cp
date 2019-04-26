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
#define  _DEFAULT_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include "cp.h"

/* copy():
 * this is the main function where the actual copying occurs
 * src and dst are file descriptors used when opening files
 */
void copy(char *source, char *destination, struct stat src_stat, struct flags flags) {
    /* check if source is a directory */
    int src, dst;
    int numbytesread;
    char buf[MAX_BUF_SIZE];
    /* open/check source (O_RDONLY) */
    if ((src = open(source, O_RDONLY)) == -1)
        error(EXIT_FAILURE, errno, "failed to open '%s'", source);
    /* open/check destination (O_WRONLY, O_CREATE, O_TRUNC) */
    /* match permissions */
    if ((dst = open(destination, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode)) == -1) {
        if (flags.force) {
            if (flags.verbose) printf("failed to open %s, removing then trying again\n", destination);
            if (remove(destination) == -1)
                error(EXIT_FAILURE, errno, "failed to remove '%s'", destination);
            if ((dst = open(destination, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode)) == -1)
                error(EXIT_FAILURE, errno, "failed to open '%s'", destination);
        }
    }
    /* read from source, write to destination */
    while ((numbytesread = read(src, buf, MAX_BUF_SIZE)) > 0) {
        if (write(dst, buf, numbytesread) != numbytesread)
            error(EXIT_FAILURE, errno, "error writing '%s'", source);
    }
    if (numbytesread == -1)
        error(EXIT_FAILURE, errno, "error reading '%s'", source);
    /* close both files */
    if (close(src) == -1)
        error(EXIT_FAILURE, errno, "failed to close '%s'", source);
    if (close(dst) == -1)
        error(EXIT_FAILURE, errno, "failed to close '%s'", destination);
}

/* do_recursion():
 * this helper function will handle walking through directories to copy files
 * when the recursion flag is set
 */
void do_recursion(char *source, char *destination, struct flags flags) {
    if (flags.verbose) printf("doing recursion on %s to %s\n", source, destination);
    DIR *src_dir;
    struct dirent *src_dirent;
    struct stat src_stat, dst_stat;
    if (lstat(source, &src_stat) == -1) {
        error(EXIT_FAILURE, errno, "failed to stat '%s'", source);
    }
    if (lstat(destination, &dst_stat) == -1) {
        if (mkdir(destination, src_stat.st_mode) == -1)
            error(EXIT_FAILURE, errno, "failed to make directory '%s'", destination);
    }
    src_dir = opendir(source);
    if (src_dir == NULL)
        error(EXIT_FAILURE, errno, "failed to open '%s'", source);
    while ((src_dirent = readdir(src_dir)) != NULL) {
        char *arg[2];
        char *filename = src_dirent->d_name;
        if (strcmp(filename, ".") == 0) continue;
        if (strcmp(filename, "..") == 0) continue;
        char path[strlen(source) + 1 + strlen(filename)];
        char *slash = (source[strlen(source) - 1] == '/') ? "" : "/";
        struct stat ent_stat;
        strcpy(path, source);
        strcat(path, slash);
        strcat(path, filename);
        if (lstat(path, &ent_stat) == -1) {
            error(EXIT_FAILURE, errno, "failed to stat '%s'", path);
        }
        if (S_ISDIR(ent_stat.st_mode)) {
            char dst_path[strlen(destination) + 1 + strlen(filename)];
            slash = (source[strlen(source) - 1] == '/') ? "" : "/";
            strcpy(dst_path, destination);
            strcat(dst_path, slash);
            strcat(dst_path, filename);
            do_recursion(path, dst_path, flags);
        }
        else {
            arg[0] = path;
            arg[1] = destination;
            copy_aux(2, arg, flags);
        }
    }
}

/* copy_aux():
 * this helper function will check if files exists and change behavior based
 * on the flags provided
 */
void copy_aux(int argc, char *argv[], struct flags flags) {
    struct stat src_stat, dst_stat;
    bool is_dir = false;
    /* check if destination is a directory */
    if (lstat(argv[argc - 1], &dst_stat) == -1) {
        /* file may not exist */
        //error(EXIT_FAILURE, errno, "failed to stat '%s'", destination);
    }
    else {
        if (S_ISDIR(dst_stat.st_mode)) {
            if (flags.verbose) printf("%s is a directory\n", argv[argc - 1]);
            is_dir = true;
        }
    }
    for (int i = 0; i < argc - 1; i++) {
        char *source = argv[i];
        char *destination;
        if (lstat(source, &src_stat) == -1)
            error(EXIT_FAILURE, errno, "failed to stat '%s'", source);
        if (S_ISDIR(src_stat.st_mode)) {
            if (flags.verbose) printf("%s is a directory\n", source);
            /* do recursion here */
            if (flags.recursion) {
                do_recursion(source, argv[argc - 1], flags);
                continue;
            }
            else {
                error(EXIT_FAILURE, errno, "'%s' is a directory", source);
            }
        }
        if (is_dir) {
            char *src_filename = basename(source);
            char *dir_name = argv[argc - 1];
            char path[strlen(dir_name) + 1 + strlen(src_filename)];
            char *slash = (dir_name[strlen(dir_name) - 1] == '/') ? "" : "/";
            strcpy(path, dir_name);
            strcat(path, slash);
            strcat(path, src_filename);
            destination = strdup(path);
        }
        else {
            destination = strdup(argv[argc - 1]);
        }
        if (destination == NULL)
            error(EXIT_FAILURE, errno, "malloc() failed");
        if (lstat(destination, &dst_stat) == -1) {
            /* file does not exist */
        }
        else if (flags.interactive) {
            if (flags.verbose) printf("%s exists, switching to interactive mode\n", destination);
            char c = 0;
            while (c != 'y' && c != 'n') {
                printf("overwrite %s? (y/N)\n", destination);
                c = tolower(getchar());
                if (c == '\n') break;
                /* flush the buffer for stdin; this prevents errors on subsequent getc calls */
                while (getchar() != '\n');
            }
            if (c == 'n' || c == '\n') {
                if (flags.verbose) printf("user chose not to overwrite %s, skipping\n", destination);
                free(destination);
                continue;
            }
        }
        else if (flags.no_clobber) {
            if (flags.verbose) printf("%s exists, will not clobber\n", destination);
            free(destination);
            continue;
        }
        else if (flags.update) {
            if (src_stat.st_mtime <= dst_stat.st_mtime) {
                if (flags.verbose) printf("%s exists and is newer than %s, skipping\n", destination, source);
                free(destination);
                continue;
            }
        }
        if (flags.verbose) printf("copying %s to %s\n", source, destination);
        if (flags.link) {
            if (link(source, destination) == -1)
                error(EXIT_FAILURE, errno, "cannot link '%s' to '%s'", source, destination);
        }
        else if (flags.symlink) {
            if (symlink(source, destination) == -1)
                error(EXIT_FAILURE, errno, "cannot symlink '%s' to '%s'", source, destination);
        }
        else
            copy(source, destination, src_stat, flags);
        free(destination);
    }
}

int main(int argc, char *argv[]) {
    struct flags flags = {0};
    int opt;

    while ((opt = getopt(argc, argv, CP_OPTS)) != -1) {
        switch (opt) {
            case 'f':
                flags.force = true;
                if (flags.verbose) printf(", force");
                break;
            case 'h':
                if (flags.verbose) printf("...skippiing to help\n");
                printf(HELP_MSG, argv[0], CP_OPTS);
                exit(EXIT_SUCCESS);
            case 'i':
                flags.interactive = true;
                if (flags.verbose) printf(", interactive");
                break;
            case 'l':
                flags.link = true;
                if (flags.verbose) printf(", hard link");
                break;
            case 'n':
                flags.no_clobber = true;
                if (flags.verbose) printf(", no Clobber");
                break;
            case 'R':
            case 'r':
                flags.recursion = true;
                if (flags.verbose) printf(", recursion");
                break;
            case 's':
                flags.symlink = true;
                if (flags.verbose) printf(", symlink");
                break;
            case 'u':
                flags.update = true;
                if (flags.verbose) printf(", update");
                break;
            case 'v':
                flags.verbose = true;
                if (flags.verbose) printf("using flags: verbose");
                break;
            default:
                fprintf(stderr, USAGE_MSG, argv[0], CP_OPTS);
                exit(EXIT_FAILURE);
        }
    }
    if (flags.verbose) printf("\n");

    if ((argc - optind) < 2) {
        fprintf(stderr, USAGE_MSG, argv[0], CP_OPTS);
        exit(EXIT_FAILURE);
    }

    /* allow for multiple source files */
    copy_aux(argc - optind, argv + optind, flags);
}
