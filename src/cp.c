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
#include <stdbool.h>
#include <string.h>

#define MAX_BUF_SIZE 1024

bool verbose = false;
bool no_clobber = false;
bool update = false;
bool interactive = false;

/* copy
 * this is the main function where the actual copying occurs
 * src and dst are file descriptors used when opening files
 * xxxpath refers to the path of the files
 * xxxfilename is the name of the files without path
 */
void copy(char *source, char *destination, struct stat src_stat) {
    /* check if source is a directory */
    int src, dst;
    int numbytesread;
    char buf[MAX_BUF_SIZE];
    /* open/check source (O_RDONLY) */
    if ((src = open(source, O_RDONLY)) == -1)
        error(EXIT_FAILURE, errno, "failed to open '%s'", source);
    /* open/check destination (O_WRONLY, O_CREATE, O_TRUNC) */
    /* match permissions */
    if ((dst = open(destination, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode)) == -1)
        error(EXIT_FAILURE, errno, "failed to open '%s'", destination);
    /* read from source, write to destination */
    while ((numbytesread = read(src, buf, MAX_BUF_SIZE)) > 0) {
        if (write(dst, buf, numbytesread) != numbytesread) {
            error(EXIT_FAILURE, errno, "error writing '%s'", source);
        }
    }
    if (numbytesread == -1)
        error(EXIT_FAILURE, errno, "error reading '%s'", source);
    /* close both files */
    if (close(src) == -1)
        error(EXIT_FAILURE, errno, "failed to close '%s'", source);
    if (close(dst) == -1)
        error(EXIT_FAILURE, errno, "failed to close '%s'", destination);
}

void copy_aux(int argc, char *argv[]) {
    struct stat src_stat, dst_stat;
    bool is_dir = false;
    /* currently unused functions
    char *src_path = dirname(source);
    char *dst_path = dirname(destination);
    char *dst_filename = basename(destination);
    */
    /* check if destination is a directory */
    if (lstat(argv[argc - 1], &dst_stat) == -1) {
        /* file may not exist */
        //error(EXIT_FAILURE, errno, "failed to stat '%s'", destination);
    }
    else {
        if (dst_stat.st_mode & S_IFDIR) {
            if (verbose) printf("%s is a directory\n", argv[argc - 1]);
            is_dir = true;
        }
    }
    for (int i = 0; i < argc - 1; i++) {
        char *source = argv[i];
        char *destination;
        if (lstat(source, &src_stat) == -1)
            error(EXIT_FAILURE, errno, "failed to stat '%s'", source);
        if (src_stat.st_mode & S_IFDIR) {
            if (verbose) printf("%s is a directory\n", source);
            /* TODO: do recursion here */
        }
        if (is_dir) {
            char *src_filename = basename(source);
            char path[sizeof(argv[argc - 1]) + 1 + sizeof(src_filename)];
            strcpy(path, argv[argc - 1]);
            strcat(path, "/");
            strcat(path, src_filename);
            destination = path;
        }
        else {
            destination = argv[argc - 1];
        }
        if (lstat(destination, &dst_stat) == -1) {
            /* file may not exist */
            //error(EXIT_FAILURE, errno, "failed to stat '%s'", destination);
        }
        else if (interactive) {
            if (verbose) printf("%s exists, switching to interactive mode\n", destination);
            printf("overwrite %s? (y/n)\n", destination);
            char c = getc(stdin);
            if (c != 'y') { /* assume 'n' for any other input */
                if (verbose) printf("user chose not to overwrite %s, skipping\n", destination);
                continue;
            }
        }
        else if (no_clobber) {
            if (verbose) printf("%s exists, will not clobber\n", destination);
            continue;
        }
        else if (update) {
            if (src_stat.st_mtime <= dst_stat.st_mtime) {
                if (verbose) printf("%s exists and is newer than %s), skipping\n", destination, source);
                continue;
            }
        }
        if (verbose) printf("copying %s to %s\n", source, destination);
        copy(source, destination, src_stat);
    }
}

int main(int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, "Rrniulv")) != -1) {
        switch (opt) {
            case 'R':
            case 'r': /* TODO */
                if (verbose) printf("Recursion\n");
                break;
            case 'n':
                no_clobber = true;
                if (verbose) printf("No Clobber\n");
                break;
            case 'i':
                interactive = true;
                if (verbose) printf("Interactive\n");
                break;
            case 'u':
                update = true;
                if (verbose) printf("Update\n");
                break;
            case 'l': /* TODO */
                if (verbose) printf("Hard Link\n");
                break;
            case 'v':
                verbose = true;
                if (verbose) printf("Verbose\n");
                break;
            default:
                fprintf(stderr, "Usage: %s [-Rrniulv] SOURCE DESTINATION\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    //printf("argc:%i, optind:%i\n", argc, optind);

    if ((argc - optind) < 2) {
        fprintf(stderr, "Usage: %s [-Rrniulv] SOURCE DESTINATION\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* allow for multiple source files */
    copy_aux(argc - optind, argv + optind);
}
