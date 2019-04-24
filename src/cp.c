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

#define MAX_BUF_SIZE 1024

bool verbose = false;

/* copy
 * this is the main function where the actual copying occurs
 * src and dst are file descriptors used when opening files
 * xxxpath refers to the path of the files
 * xxxfilename is the name of the files without path
 */
void copy(char *source, char *destination) {
    int src, dst;
    struct stat srcstatbuf, dststatbuf;
    /* currently unused functions
    char *srcpath = dirname(source);
    char *dstpath = dirname(destination);
    char *srcfilename = basename(source);
    char *dstfilename = basename(destination);
    */
    /* check if source is a directory */
    if (lstat(source, &srcstatbuf) == -1)
        error(EXIT_FAILURE, errno, "failed to stat '%s'", source);
    if (srcstatbuf.st_mode & S_IFDIR) {
        if (verbose) printf("%s is a directory\n", source);
    }
    else {
        /* open/check source (O_RDONLY) */
        if ((src = open(source, O_RDONLY)) == -1)
            error(EXIT_FAILURE, errno, "failed to open '%s'", source);
    }
    /* check if destination is a directory */
    if (lstat(destination, &dststatbuf) == -1) {
        /* file may not exist */
        //error(EXIT_FAILURE, errno, "failed to stat '%s'", destination);
    }
    else {
        if (dststatbuf.st_mode & S_IFDIR) {
            if (verbose) printf("%s is a directory\n", destination);
        }
    }
    /* open/check destination (O_WRONLY, O_CREATE, O_TRUNC) */
    /* match permissions */
    if ((dst = open(destination, O_WRONLY | O_CREAT | O_TRUNC, srcstatbuf.st_mode)) == -1)
        error(EXIT_FAILURE, errno, "failed to open '%s'", destination);
    /* read from source, write to destination */
    int numbytesread;
    char buf[MAX_BUF_SIZE];
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
    char *destination = argv[argc - 1];
    for (int i = 0; i < argc - 1; i++) {
        char *source = argv[i];
        if (verbose) printf("copying %s to %s\n", source, destination);
        copy(source, destination);
    }
}

int main(int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, "Rrniulv")) != -1) {
        switch (opt) {
            case 'R':
            case 'r':
                if (verbose) printf("Recursion\n");
                break;
            case 'n':
                if (verbose) printf("No Clobber\n");
                break;
            case 'i':
                if (verbose) printf("Interactive\n");
                break;
            case 'u':
                if (verbose) printf("Update\n");
                break;
            case 'l':
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
