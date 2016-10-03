#include <stdio.h>
#include <string.h>

void init(const char* str, int next[], int n)
{
    int i, k = 0;
    next[1] = 0;
    for (i = 2; i <= n; ++i) {
        while (k > 0 && str[k+1] != str[i]) {
            k = next[k];
        }
        if (str[k+1] == str[i]) {
            k = k+1;
        }
        next[i] = k;
    }
}

void match(const char* str, int sn, const char*pat, int pn)
{
    int next[128] = {0};
    init(pat, next, pn);
    int k = 0, i;
    for (i = 1; i <= sn; ++i) {
        while (k > 0 && pat[k+1] != str[i]) {
            k = next[k];
        }
        if (pat[k+1] == str[i]) {
            k++;
        }
        if (k == pn) {
            printf("%s\n", pat+1);
            k = 0;
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage:%s src pat\n", argv[0]);
        return -1;
    }
    match(argv[1], strlen(argv[1])-1, argv[2], strlen(argv[2])-1);
    return 0;
}
