#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

void find(char *path, char *target, char *exec_cmd) {
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
    
    switch (st.type) {
        case T_FILE: {
            p = strrchr(path, '/');
            if (p == 0) p = path;
            else p++;
            if (strcmp(p, target) == 0) {
                if (exec_cmd != 0) {
                    int pid = fork();
                    if (pid == 0) {
                        char *argv[] = {exec_cmd, path, 0};
                        exec(exec_cmd, argv);
                        fprintf(2, "find: exec failed\n");
                        exit(1);
                    } else {
                        wait(0);
                    }
                } else {
                    printf("%s\n", path);
                }
            }
            break;
        }
        case T_DIR: {
            if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
                fprintf(2, "find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            
            while (read(fd, &de, sizeof(de)) == sizeof(de)) {
                if (de.inum == 0) continue;
                if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                    continue;
                }
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buf, target, exec_cmd);
            }
            break;
        }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(2, "Usage: find <path> <filename> [-exec cmd]\n");
        exit(1);
    }
    
    char *exec_cmd = 0;
    if (argc >= 4 && strcmp(argv[3], "-exec") == 0) {
        exec_cmd = argv[4];
    }
    
    find(argv[1], argv[2], exec_cmd);
    exit(0);
}
