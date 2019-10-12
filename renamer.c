#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define MAXTREE 512000

struct smalldirent {
    unsigned char d_type;
    char d_name[256];
};

struct badchar {
    char new;       // max 0x00000080
    char extra;     // max 0x00008000
    char bad;       // max 0x00800000
    char skip;      // max 0x80000000
};

struct smalldirent tree[MAXTREE];
int count;
int dryrun;

struct badchar is_bad_bashchar(char c)
{
    struct badchar y;

    if ((c > 0 && c < 32) || (c > 126 && c <= 255) || (c < 0)) {
        // strange characters, replace with nothing
        y.bad = 1;
        y.skip = 1;
    } else if (c == ' ' || c == '|' || c == '&' || c == ';' || c == '(' ||
               c == ')' || c == '<' || c == '>' || c == '!' || c == '[' ||
               c == ']' || c == '{' || c == '}' || c == ':' || c == '?' ||
               c == '\'' ||
               c == '=' || c == '*' || c == '/' || c == '"' || c == '\\' ) {
        // strange characters, replace with underscore
        y.new = '_';
        y.skip = 0;
        y.bad = 1;
    } else {
        y.bad = 0;
    }

    return y;
}

int is_bad_bash(char *s)
{
    struct badchar y;

    while (*s) {
        y = is_bad_bashchar(*s++);
        if (y.bad) {
            return 1;
        }
    }

    return 0;
}

void good_name(char *dst, char *src)
{
    struct badchar y;

    while (*src) {
        y = is_bad_bashchar(*src);
        if (y.bad) {
            if (!y.skip) {
                if (y.extra) {
                    *dst++ = y.new;
                    *dst++ = y.extra;
                } else {
                    *dst++ = y.new;
                }
            }
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

void frename(char *dirname)
{
    char *rp;
    char rpath[PATH_MAX];
    char npath[PATH_MAX];
    char xpath[PATH_MAX];
    char ypath[PATH_MAX];
    struct stat buf;
    int rc;
    DIR *d;
    struct dirent *td;
    int start;
    int stop;
    int i;

    rp = realpath(dirname, rpath);

    if (rp == NULL) {
        fprintf(stderr, "could not resolve \"%s\" into absolute path\n", dirname);
        abort();
    }

    rc = stat(rpath, &buf);

    if (rc != 0) {
        fprintf(stderr, "could not process \"%s\"\n", rpath);
        abort();
    }
    
    if (!S_ISDIR(buf.st_mode)) {
        fprintf(stderr, "given directory \"%s\" could not be stat'd\n", rpath);
        abort();
    }

    d = opendir(rpath);
    if (d == NULL) {
        fprintf(stderr, "could not open dir \"%s\"\n", rpath);
        abort();
    }

    start = count;
    while ((td = readdir(d))) {
        if (count == MAXTREE) {
            fprintf(stderr, "Not enough memory for directory entires, all OK, stopping here.\n");
            fprintf(stderr, "Please run again on a smaller section if there are still files remaining.\n");
            closedir(d);
            exit(0);
        } else {
            tree[count].d_type = td->d_type;
            strncpy(tree[count].d_name, td->d_name, 256);
            tree[count].d_name[255] = '\0';
            count++;
        }
    }
    stop = count;
    closedir(d);

    for (i=start ; i<stop ; i++) {
        if (tree[i].d_name[0] == '.') {
            // Intentionally skip dotfiles, including current directory parent
            // directory and any hidden files. We never want to traverse these
            // directories. Hidden dirs mostly contain program configuration
            // (addressed by name). If a user wants to process them, he must
            // enter that directory manually.
            continue;
        }

        if (is_bad_bash(tree[i].d_name) && (tree[i].d_type == DT_DIR || tree[i].d_type == DT_REG)) {
            snprintf(xpath, PATH_MAX, "%s/%s", rpath, tree[i].d_name);
            good_name(npath, tree[i].d_name);
            snprintf(ypath, PATH_MAX, "%s/%s", rpath, npath);
            if (tree[i].d_type == DT_DIR) {
                printf("%srename directory %s to %s\n", dryrun ? "[dryrun] " : "", xpath, ypath);
            } else if (tree[i].d_type == DT_REG) {
                printf("%srename file %s to %s\n",  dryrun ? "[dryrun] " : "", xpath, ypath);
            }

            rc = stat(ypath, &buf);

            if (rc == 0) {
                fprintf(stderr, "target name \"%s\" already exists, skipping\n", ypath);
            } else if (!dryrun) {
                if (renameat(0, xpath, 0, ypath) == 0) {
                    strcpy(tree[i].d_name, npath);
                } else {
                    fprintf(stderr, "failed to rename %s to %s\n", xpath, ypath);
                    abort();
                }
            }
        }

        if (tree[i].d_type == DT_DIR) {
            snprintf(npath, PATH_MAX, "%s/%s", rpath, tree[i].d_name);
            frename(npath);
        }
    }
}

int main(int argc, char *argv[])
{
    int i;
    if (argc >= 2) {
        if (strcmp(argv[1], "commit") == 0) {
            dryrun = 0;
        } else if (strcmp(argv[1], "dryrun") == 0) {
            dryrun = 1;
        } else {
            goto usage;
        }
    } else {
        goto usage;
    }
    for (i=2 ; i<argc ; i++) {
        count = 0;
        frename(argv[i]);
    }
    return 0;
usage:
    printf("\n"); 
    printf("    usage: %s <commit|dryrun> path0 path1 .. pathN\n", argv[0]);
    printf("\n"); 
    printf(" Remove characters that needs escaping in bash in filenames recursively\n");
    printf(" in given list of directories.\n");
    printf("\n"); 
    printf(" Please note that dryrun will not print double filename collisions\n");
    printf(" handling at all, e.g: bad:file.txt bad'file.txt will both translate to\n");
    printf(" the same target name - but the problem will be detected in commit mode\n");
    printf(" and nothing will be overwritten, although the first rename in that\n");
    printf(" scenario will take place.\n");
    printf("\n"); 
    printf(" Bugs/limitations/behaviour:\n"); 
    printf("  - commit mode will never overwrite existing files\n");
    printf("  - commit mode might generate double target names, \n");
    printf("    but will still not overwrite at second rename.\n");
    printf("  - race-condition *may* occur between stat() and rename()\n");
    printf("    at does-file-already-exists-check time\n");
    printf("  - trees are traversed by recursion, stack may overflow\n");
    printf("    but you'll surely notice :-)\n");
    printf("  - %s keep all dirents in memory, run on smaller trees\n", argv[0]);
    printf("    if the memory limit of %lu bytes is hit\n", MAXTREE*sizeof(struct dirent));
    printf("  - will not process anything starting with a dot\n");
    return 0;
}

