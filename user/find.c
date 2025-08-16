#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"


/******************************************************************************
 * Key Idea: 
 * 1. Open the file/directory
 * 2. Use fstat to get file information
 * 3. Switch on the file type:
 *    - For regular files: process as needed
 *    - For directories (T_DIR):
 *      a. Read directory entries
 *      b. Recurse into subdirectories
 ******************************************************************************/
char*
fmtname(char *path) 
{
    static char buf[DIRSIZ+1];
    char *p;

    for (p=path+strlen(path); p>= path && *p != '/'; p--)
        ;
    p++;

    if (strlen(p) >= DIRSIZ)
        return p;

    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
    return buf;
}

void 
find(char *path, char *pattern)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, O_RDONLY)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type) {
    case T_DEVICE:
    case T_FILE:
        // Check if filename matches pattern
        p = path + strlen(path);
        while (p >= path && *p != '/')
            p--;
        p++;
        if (strcmp(p, pattern) == 0)
            printf("%s\n", path);
        break;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0)
                continue;
            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;

            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if (stat(buf, &st) < 0) {
                printf("find: cannot stat %s\n", buf);
                continue;
            }

            // Recursively search subdirectories and files
            find(buf, pattern);
        }
        break;
    }
    close(fd);
}

int 
main(int argc, char *argv[]) 
{
    if (argc < 3) {
        fprintf(2, "usage: find <path> <name>\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}

