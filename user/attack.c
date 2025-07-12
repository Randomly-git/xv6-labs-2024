#include "kernel/types.h"
#include "user/user.h"
#include "kernel/riscv.h"

#define PGSIZE 4096
#define TOTAL_PAGES 32

int is_valid_secret(char *s) {
    const char valid_chars[] = "./abcdef";
    for (int i = 0; i < 7; i++) {
        char c = s[i];
        int is_valid = 0;
        for (int j = 0; j < 8; j++) {
            if (c == valid_chars[j]) {
                is_valid = 1;
                break;
            }
        }
        if (!is_valid) return 0;
    }
    return (s[7] == '\0') ? 1 : 0;
}

int main(void) {
    char *mem = sbrk(PGSIZE * TOTAL_PAGES);
    if (mem == (char*)-1) {
        printf("sbrk failed\n");
        exit(1);
    }

    for (int i = 0; i < TOTAL_PAGES; i++) {
        char *candidate = mem + i * PGSIZE + 32;
        if (is_valid_secret(candidate)) {
            write(2, candidate, 8);  
        }
    }

    exit(1);  
}