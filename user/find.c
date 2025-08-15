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
fmtname(char *path) 
{
    static char buf[DIRSIZ+1];
}


