#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int is_separator(char c) {
    return c == ' ' || c == '-' || c == '\r' || c == '\t' || 
           c == '\n' || c == '.' || c == '/' || c == ',';
}

int is_multiple(int n) {
    return n % 5 == 0 || n % 6 == 0;
}

void process_file(int fd) {
    char buf[1];
    int num = 0;
    int in_num = 0;
    
    while (read(fd, buf, 1) > 0) {
        char c = buf[0];
        if (c >= '0' && c <= '9') {
            num = num * 10 + (c - '0');
            in_num = 1;
        } else if (is_separator(c)) {
            if (in_num && is_multiple(num)) {
                printf("%d\n", num);
            }
            num = 0;
            in_num = 0;
        }
    }
    if (in_num && is_multiple(num)) {
        printf("%d\n", num);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        process_file(0);
        exit(0);
    }
    
    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            fprintf(2, "sixfive: cannot open %s\n", argv[i]);
            exit(1);
        }
        process_file(fd);
        close(fd);
    }
    exit(0);
}
