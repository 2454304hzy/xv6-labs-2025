#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        // Example format strings and data
        int num1 = 61810;
        int num2 = 2025;
        char *str = "a string";
        char *another = "another";
        unsigned long long addr = 0xBD0;
        struct {
            int a;
            int b;
            char c;
            char *d;
        } s = {1819438967, 100, 'z', "xyzzy"};
        char hello[] = "hello";
        
        printf("Example 1:\n");
        memdump("i", (char *)&num1);
        memdump("i", (char *)&num2);
        
        printf("Example 2:\n");
        memdump("s", (char *)&str);
        
        printf("Example 3:\n");
        memdump("S", another);
        
        printf("Example 4:\n");
        memdump("phic", (char *)&addr);
        memdump("is", (char *)&s);
        
        printf("Example 5:\n");
        memdump("cccc", hello);
        memdump("c", hello);
        memdump("c", hello+1);
        memdump("c", hello+2);
        memdump("c", hello+3);
        memdump("c", hello+4);
    } else {
        char data[1024];
        int n = 0;
        int c;
        while ((c = read(0, data + n, 1)) > 0) {
            n++;
        }
        data[n] = 0;
        memdump(argv[1], data);
    }
    exit(0);
}

void
memdump(char *fmt, char *data)
{
    char *p = fmt;
    char *cur = data;
    
    while (*p) {
        switch (*p) {
            case 'i': {
                int val = *(int *)cur;
                printf("%d\n", val);
                cur += 4;
                break;
            }
            case 'p': {
                unsigned long long val = *(unsigned long long *)cur;
                printf("%llx\n", val);
                cur += 8;
                break;
            }
            case 'h': {
                short val = *(short *)cur;
                printf("%d\n", val);
                cur += 2;
                break;
            }
            case 'c': {
                char val = *cur;
                printf("%c\n", val);
                cur += 1;
                break;
            }
            case 's': {
                char **ptr = (char **)cur;
                printf("%s\n", *ptr);
                cur += 8;
                break;
            }
            case 'S': {
                printf("%s\n", cur);
                cur += strlen(cur) + 1;
                break;
            }
            default: {
                cur++;
                break;
            }
        }
        p++;
    }
}
