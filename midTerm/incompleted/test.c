#include <stdio.h>
#include <stdlib.h>

int* arr[10];
int idx = 0;

void declare(int* x) {
    arr[idx] = x;
    idx++;
}


int main(int argc, char const* argv[])
{
    int* x = NULL;
    for (int i = 0; i < 10; i++) {
        x = (int*)malloc(sizeof(int));
        declare(x);
    }
    for (int i = 0; i < 10; i++) {
        free(arr[i]);
    }
    return 0;
}
