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
#include <dirent.h>
#include <ctype.h>

#define MAX_BUF_SIZE 1024

bool verbose = false;
bool no_clobber = false;
bool update = false;
bool interactive = false;
bool recursion = false;

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
            /* do recursion here */
            if (recursion) {
                if (verbose) printf("doing recursion on %s\n", source);
                DIR *src_dir;
                struct dirent *src_dirent;
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
                    strcpy(path, source);
                    strcat(path, slash);
                    strcat(path, filename);
                    arg[0] = path;
                    arg[1] = argv[argc - 1];
                    copy_aux(2, arg);
                }
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
            /* file may not exist */
            //error(EXIT_FAILURE, errno, "failed to stat '%s'", destination);
        }
        else if (interactive) {
            if (verbose) printf("%s exists, switching to interactive mode\n", destination);
            char c = 0;
            while (c != 'y' && c != 'n') {
                printf("overwrite %s? (y/N)\n", destination);
                c = tolower(getchar());
                if (c == '\n') break;
                /* flush the buffer for stdin; this prevents errors on subsequent getc calls */
                while (getchar() != '\n');
            }
            if (c == 'n' || c == '\n') {
                if (verbose) printf("user chose not to overwrite %s, skipping\n", destination);
                free(destination);
                continue;
            }
        }
        else if (no_clobber) {
            if (verbose) printf("%s exists, will not clobber\n", destination);
            free(destination);
            continue;
        }
        else if (update) {
            if (src_stat.st_mtime <= dst_stat.st_mtime) {
                if (verbose) printf("%s exists and is newer than %s), skipping\n", destination, source);
                free(destination);
                continue;
            }
        }
        if (verbose) printf("copying %s to %s\n", source, destination);
        copy(source, destination, src_stat);
        free(destination);
    }
}

int main(int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, "Rrniulv")) != -1) {
        switch (opt) {
            case 'R':
            case 'r': /* TODO: create directories */
                recursion = true;
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
