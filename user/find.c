#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

/********************** Regexp matcher from Kernighan & Pike,******************************/
// The Practice of Programming, Chapter 9, or
// https://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html

int matchhere(char*, char*);
int matchstar(int, char*, char*);

// ^ -> match start of string
// $ -> match end of string
// . -> match any one character
// * -> match zero or more of the preceding character

/* match: search for regexp anywhere in text */
int match(char *re, char *text)
{
    if (re[0] == '^')
        return matchhere(re+1, text);
    do { // must look even if string is empty
        if (matchhere(re, text))
            return 1;
    } while(*text++ != '\0');
    return 0;
}

/* matchhere: search for regexp at beginning of text */
int matchhere(char *re, char *text)
{
    if (re[0] == '\0')
        return 1;
    if (re[1] == '*')
        return matchstar(re[0], re+2, text);
    if (re[0] == '$' && re[1] == '\0')
        return *text == '\0';
    if (*text != '\0' && (re[0] == '.' || re[0] == *text))
        return matchhere(re+1, text+1);
    return 0;
}

/* matchstar: search for c*regexp at beginning of text */
int matchstar(int c, char *regexp, char *text)
{
    do {    /* a * matches zero or more instances */
        if (matchhere(regexp, text))
            return 1;
    // text pointer must advance, hence the order of comparison matters
    } while (*text != '\0' && (*text++ == c || c == '.')); 
    return 0;
}

/*****************************************************************
 * Glob-Style Matcher
 * '*' -> matches zero or more chars
 * '?' -> matches any single char
 * no anchors (^, $) needed -> behaves like strcmp with wildcards
 *****************************************************************/
int globmatch(const char *pattern, const char *text) {
    while (*pattern) {
        if (*pattern == '*') {
            // Collapse multiple '*' in a row
            while (*(pattern + 1) == '*')
                pattern++;

            // Try to match rest of pattern at every tail of text
            pattern++;
            if (*pattern == '\0')
                return 1; // trailing '*' matches everything
            while (*text) {
                if (globmatch(pattern, text))
                    return 1;
                text++;
            }
            return 0;
        } else if (*pattern == '?') {
            if (*text == '\0') // we expect one char matching, but the text buf ends
                return 0;
            pattern++;
            text++;
        } else {
            if (*pattern != *text) // oops -> no match
                return 0;
            pattern++;
            text++;
        }
    }
    return *text == '\0';
}


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
        if (match(p, pattern))
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

            //// if the name matches, print full path
            //if (match(pattern, de.name)) {
            //    printf("%s\n", buf);
            //}
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
    // Auto expand user's pattern: 'foo' -> '^.*foo.*$'
    char wrapped[256];
    char *p = wrapped;
    
    strcpy(wrapped, "^.*");
    p += strlen("^.*");

    strcpy(p, argv[2]);
    p += strlen(argv[2]);

    strcpy(p, ".*$");
    find(argv[1], wrapped); // wrapped ==> ^.*<pattern>.*$
    exit(0);
}

