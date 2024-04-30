#include <stdio.h>
int main() {
    int a[3][5] = {{ 10, 20, 30, 40, 50 }, { 60, 70, 80, 90, 100 }, { 101, 111, 121, 131, 141 }};

    printf("%p\n", **a);
    printf("%p\n", **a+1);
    printf("%p\n", **(a+1));
    printf("%p\n", *(*a+1));
    printf("%p\n", **a+2);
    printf("%p\n", **(a+2));
    printf("%p\n", *(*a+2));
    
    printf("%d\n", **a);10
    printf("%d\n", **a+1);11
    printf("%d\n", **(a+1));60
    printf("%d\n", *(*a+1)); 20
    printf("%d\n", **a+2); 12
    printf("%d\n", **(a+2)); 101
    printf("%d\n", *(*a+2)); 30

    int *b[2][5] = {{ 0, 1, 2, 3, 4 }, { 5, 6, 7, 8, 9 }};

    printf("%p\n", **b);
    printf("%p\n", **b+1);
    printf("%p\n", **b+2);
    printf("%d\n", **b);
    printf("%d\n", **b+1);
    printf("%d\n", **b+2);
}

