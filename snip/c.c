#include <stdio.h>

static int s_int;
extern int e_int;

static void s_func(int *val)
{
    *val = 0x00000001;
}

extern void e_func(int *val);

int main()
{
    int a, b;
    s_func(a);
    e_func(&b);
    return a + b;
}

//////////////

int eg_int;

void e_func(int *val)
{
    *val = 0x00000200;
}


